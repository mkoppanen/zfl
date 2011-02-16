/*  =========================================================================
    zfl_rpc.c - client side reliable RPC

    Client side API for implementing reliable remote procedure calls.
    Use in conjunction with the zfl_rpcd class for the server side.

    -------------------------------------------------------------------------
    Copyright (c) 1991-2010 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the ZeroMQ Function Library: http://zfl.zeromq.org

    This is free software; you can redistribute it and/or modify it under the
    terms of the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your option)
    any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABIL-
    ITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
    Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include <zmq.h>
#include "../include/zfl_prelude.h"
#include "../include/zfl_hash.h"
#include "../include/zfl_list.h"
#include "../include/zfl_msg.h"
#include "../include/zfl_thread.h"
#include "../include/zfl_rpc.h"

//  Heartbeat rate (in microseconds)
#define HEARTBEAT_INTERVAL      500000

//  Maximum time we wait for server's reply (in microseconds)
#define MAX_PROCESSING_TIME     2000000

//  Structure of our class

struct _zfl_rpc {
    void
        *data_socket,           //  Used to send requests/receive replies
        *ctrl_socket;           //  Used to control RPC thread
    zfl_thread_t
        *thread;                //  API thread
};


//  Represents server as viewed by client
//  We provide a minimal local constructor and destructor

typedef struct {
    char
        *server_id;
    int
        alive;                  //  True iff server's heart is beating
    uint64_t
        heartbeat_deadline;     //  Until when we wait for heartbeat
} server_t;

//  Allocate and initialize a new server object
static server_t *
s_server_new (char *server_id)
{
    server_t *self = (server_t *) zmalloc (sizeof (server_t));
    self->server_id = strdup (server_id);
    return (self);
}

//  Has to be compatible with free() for zfl_hash_freefn
static void
s_server_destroy (void *self)
{
    if (self) {
        free (((server_t *) self)->server_id);
        free (self);
    }
}


//  Internal structure used by RPC thread

typedef struct {
    void
        *frontend,              //  Used to communicate with application
        *backend,               //  Used to communicate with RPC server
        *control;               //  Used to control RPC server
    zfl_list_t
        *servers,               //  Servers client is connected to
        *alive_servers,         //  List of alive server
        *lru_queue;             //  Servers ready to serve (in LRU order)
    zfl_hash_t
        *registry;              //  Maps server names to pointer to server struct
    unsigned long
        sequence_nr;            //  Sequence number allocator
    zfl_msg_t
        *request;               //  Pending request or NULL
    server_t
        *current_server;        //  Server processing the last request or NULL
    uint64_t
        next_heartbeat,         //  Time of next heartbeat
        processing_deadline;    //  Until when we wait for result
} rpc_t;


//  Used to pass arguments on RPC thread creation

typedef struct {
    void
        *zmq_context;
    char
        *data_endpoint,
        *ctrl_endpoint;
} thread_args_t;


//  --------------------------------------------------------------------------
//  Return current time (in microseconds)
//	This code should go into a zfl_time class

static uint64_t
s_now (void)
{
#if (defined (__UNIX__))
    struct timeval tv;
    int rc = gettimeofday (&tv, NULL);
    assert (rc == 0);
    return (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
#elif (defined (__WINDOWS__))
    SYSTEMTIME st;
    GetSystemTime (&st);
    return (uint64_t) st.wSecond * 1000000 + st.wMilliseconds * 1000;
#else
#   error "zfl_rpc does not compile on this system"
#endif
}


//  --------------------------------------------------------------------------
//  Handle message received from a server

static void
s_backend_event (rpc_t *rpc)
{
    zfl_msg_t *msg = zfl_msg_recv (rpc->backend);
    assert (msg);

    char *server_id = zfl_msg_unwrap (msg);
    server_t *server = (server_t *) zfl_hash_lookup (rpc->registry, server_id);
    assert (server);
    free (server_id);

    if (zfl_msg_parts (msg) == 0) {
        //  Heartbeat signal
        if (server->alive)
            zfl_list_remove (rpc->alive_servers, server);
        else {
            zfl_list_append (rpc->lru_queue, server);
            server->alive = 1;
        }
        server->heartbeat_deadline = s_now () + HEARTBEAT_INTERVAL;
        zfl_list_append (rpc->alive_servers, server);
    }
    else
    if (zfl_msg_parts (msg) == 2) {
        if (server == rpc->current_server) {
            assert (rpc->request);
            char *request_id = zfl_msg_pop (msg);
            unsigned long sequence_nr = strtoul (request_id, NULL, 10);
            free (request_id);
            if (sequence_nr == rpc->sequence_nr) {
                zfl_msg_body_set (rpc->request, zfl_msg_body (msg));
                zfl_msg_send (&rpc->request, rpc->frontend);
                rpc->sequence_nr++;
                rpc->current_server = NULL;
            }
        }
    }
    zfl_msg_destroy (&msg);
}


//  --------------------------------------------------------------------------
//  Receive request from client

static void
s_frontend_event (rpc_t *rpc)
{
    zfl_msg_t *msg = zfl_msg_recv (rpc->frontend);
    assert (msg);

    assert (zfl_list_size (rpc->servers) > 0);
    assert (rpc->request == NULL);
    assert (rpc->current_server == NULL);

    //  Save the request now. It will be handled in main loop.
    rpc->request = msg;
}


//  --------------------------------------------------------------------------
//  Handle control message from application.
//  Returns TRUE when the received message contains `stop` command

static Bool
s_control_event (rpc_t *rpc)
{
    int rc;
    Bool stopped = FALSE;

    zfl_msg_t *msg = zfl_msg_recv (rpc->control);
    assert (msg);
    assert (zfl_msg_parts (msg) > 0);

    char *command = zfl_msg_pop (msg);
    if (strcmp (command, "stop") == 0) {
        assert (zfl_msg_parts (msg) == 0);
        stopped = TRUE;
    }
    else {
        assert (strcmp (command, "connect") == 0);
        assert (zfl_msg_parts (msg) == 2);
        char *server_id = zfl_msg_pop (msg);
        char *endpoint = zfl_msg_pop (msg);

        //  Insert new server, which can't already exist
        assert (zfl_hash_lookup (rpc->registry, server_id) == NULL);
        rc = zmq_connect (rpc->backend, endpoint);
        assert (rc == 0);

        server_t *server = s_server_new (server_id);
        rc = zfl_hash_insert (rpc->registry, server_id, server);
        assert (rc == 0);
        zfl_hash_freefn (rpc->registry, server_id, s_server_destroy);
        zfl_list_append (rpc->servers, server);

        //  Send response
        zfl_msg_t *response = zfl_msg_new ();
        zfl_msg_push (response, "ok");
        zfl_msg_send (&response, rpc->control);
        free (server_id);
        free (endpoint);
    }
    zfl_msg_destroy (&msg);
    free (command);

    return stopped;
}

//  --------------------------------------------------------------------------
//  Send heartbeat message to connected servers.

static void
s_send_heartbeat (rpc_t *rpc)
{
    zfl_list_t *servers = zfl_list_copy (rpc->servers);
    assert (servers);

    while (zfl_list_size (servers) > 0) {
        server_t *server = (server_t *) zfl_list_first (servers);
        zfl_list_remove (servers, server);

        //  Prepare and send out heartbeat message
        zfl_msg_t *msg = zfl_msg_new ();
        assert (msg);
        zfl_msg_wrap (msg, server->server_id, "");
        zfl_msg_send (&msg, rpc->backend);
    }
    zfl_list_destroy (&servers);
}


//  --------------------------------------------------------------------------
//  Main RPC client thread; this is what we talk to via other methods

static void *
s_rpc_thread (void *arg)
{
    thread_args_t *thread_args = (thread_args_t *) arg;
    int rc;

    //  Grab a new rpc_t structure to hold our thread state
    rpc_t *rpc = (rpc_t *) zmalloc (sizeof (rpc_t));
    memset (rpc, 0, sizeof (rpc_t));

    rpc->frontend = zmq_socket (thread_args->zmq_context, ZMQ_REP);
    assert (rpc->frontend);
    rpc->control  = zmq_socket (thread_args->zmq_context, ZMQ_REP);
    assert (rpc->control);
    rpc->backend  = zmq_socket (thread_args->zmq_context, ZMQ_XREP);
    assert (rpc->backend);
    rc = zmq_connect (rpc->frontend, thread_args->data_endpoint);
    assert (rc == 0);
    rc = zmq_connect (rpc->control, thread_args->ctrl_endpoint);
    assert (rc == 0);

    free (thread_args->data_endpoint);
    free (thread_args->ctrl_endpoint);
    free (thread_args);

    rpc->servers = zfl_list_new ();
    assert (rpc->servers);
    rpc->alive_servers = zfl_list_new ();
    assert (rpc->alive_servers);
    rpc->lru_queue = zfl_list_new ();
    assert (rpc->lru_queue);
    rpc->registry = zfl_hash_new ();
    assert (rpc->registry);

    rpc->next_heartbeat = s_now ();

    //  Controls how long we wait for message. Updated during processing.
    long poll_timeout = -1;

    //  Controls the server loop
    int stopped = 0;
    while (!stopped) {
        zmq_pollitem_t items [] = {
            //  Poll for server activity
            {rpc->backend, -1, ZMQ_POLLIN, 0},
            //  Poll for client activity
            {rpc->frontend, -1, ZMQ_POLLIN, 0},
            //  Poll for new control command
            {rpc->control, -1, ZMQ_POLLIN, 0}
        };

        rc = zmq_poll (items, 3, poll_timeout);
        assert (rc != -1);

        if (items [0].revents & ZMQ_POLLIN)
            // Either response or heartbeat signal
            s_backend_event (rpc);
        if (items [1].revents & ZMQ_POLLIN)
            // select server and forward the request to him
            s_frontend_event (rpc);
        if (items [2].revents & ZMQ_POLLIN)
            //  Either connect or stop message
            stopped = s_control_event (rpc);

        //  Get current time
        uint64_t now = s_now ();

        //  Time for heartbeat?
        if (now >= rpc->next_heartbeat) {
            s_send_heartbeat (rpc);
            rpc->next_heartbeat = now + HEARTBEAT_INTERVAL;
        }

        //  Check for dead servers
        while (zfl_list_size (rpc->alive_servers) > 0) {
            server_t *server = (server_t *) zfl_list_first (rpc->alive_servers);
            if (now < server->heartbeat_deadline)
                break;
            zfl_list_remove (rpc->alive_servers, server);
            zfl_list_remove (rpc->lru_queue, server);
            server->alive = 0;
        }

        //  Check for late response
        if (rpc->current_server)
            if (now >= rpc->processing_deadline)
                rpc->current_server = NULL;

        //  Forward pending request to the least recently used server
        if (rpc->request && !rpc->current_server) {
            if (zfl_list_size (rpc->lru_queue) > 0) {
                server_t *server = (server_t *) zfl_list_first (rpc->lru_queue);
                zfl_msg_t *msg = zfl_msg_new ();

                //  Copy the request without address envelope
                zfl_msg_body_set (msg, zfl_msg_body (rpc->request));

                //  Add request ID
                char request_id [16];
                sprintf (request_id, "%lu", rpc->sequence_nr);
                zfl_msg_push (msg, request_id);

                //  Add address envelope
                zfl_msg_wrap (msg, server->server_id, NULL);
                zfl_msg_send (&msg, rpc->backend);

                rpc->current_server = server;
                rpc->processing_deadline = now + MAX_PROCESSING_TIME;

                //  Move the server at the end of the LRU queue
                zfl_list_remove (rpc->lru_queue, server);
                zfl_list_append (rpc->lru_queue, server);
            }
        }

        //  How long to poll for next message?
        if (zfl_list_size (rpc->servers) == 0)
            //  If there is no server, we can wait infinitely
            poll_timeout = -1;
        else {
            //  Wait at most till next heartbeat
            poll_timeout = (long) (rpc->next_heartbeat - now);

            if (zfl_list_size (rpc->alive_servers) > 0) {
                server_t *server = (server_t *) zfl_list_first (rpc->alive_servers);
                if (server->heartbeat_deadline < now + poll_timeout)
                    poll_timeout = (long) (server->heartbeat_deadline - now);
            }
            if (rpc->current_server)
                if (rpc->processing_deadline < now + poll_timeout)
                    //  Wait at most till next processing deadline
                    poll_timeout = (long) (rpc->processing_deadline - now);
        }
    }

    //  Close sockets
    zmq_close (rpc->frontend);
    zmq_close (rpc->backend);
    zmq_close (rpc->control);

    //  Destroy data structures
    zfl_list_destroy (&rpc->servers);
    zfl_list_destroy (&rpc->alive_servers);
    zfl_list_destroy (&rpc->lru_queue);
    zfl_hash_destroy (&rpc->registry);

    free (rpc);

    return NULL;
}


//  --------------------------------------------------------------------------
//  Constructor

zfl_rpc_t *
zfl_rpc_new (void *zmq_context)
{
    int rc;
    char
        data_endpoint [32],
        ctrl_endpoint [32];

    zfl_rpc_t *self = (zfl_rpc_t *) zmalloc (sizeof (zfl_rpc_t));
    self->data_socket = zmq_socket (zmq_context, ZMQ_REQ);
    assert (self->data_socket);
    self->ctrl_socket = zmq_socket (zmq_context, ZMQ_REQ);
    assert (self->ctrl_socket);

    //  Allow any number of RPC clients to exist in our process
	srandom ((unsigned) time (NULL));
    while (1) {
        int port_number = randof (0x10000);
        sprintf (data_endpoint, "inproc://rpc/%04X/data", port_number);
        sprintf (ctrl_endpoint, "inproc://rpc/%04X/ctrl", port_number);
        rc = zmq_bind (self->data_socket, data_endpoint);
        if (rc == 0)
            break;
    }
    rc = zmq_bind (self->ctrl_socket, ctrl_endpoint);
    assert (rc == 0);

    thread_args_t *thread_args = (thread_args_t *) zmalloc (sizeof *thread_args);
    memset (thread_args, 0, sizeof (thread_args_t));
    thread_args->zmq_context = zmq_context;
    thread_args->data_endpoint = strdup (data_endpoint);
    thread_args->ctrl_endpoint = strdup (ctrl_endpoint);

    self->thread = zfl_thread_new (s_rpc_thread, thread_args);
    assert (self->thread);

    return self;
}


//  --------------------------------------------------------------------------
//  Destructor

void
zfl_rpc_destroy (zfl_rpc_t **self_p)
{
    zfl_rpc_t *self = *self_p;
    if (!self)
        return;

    zfl_msg_t *stop_request = zfl_msg_new ();
    assert (stop_request);
    zfl_msg_push (stop_request, "stop");
    zfl_msg_send (&stop_request, self->ctrl_socket);

    zfl_thread_wait (self->thread);
    zfl_thread_destroy (&self->thread);

    zmq_close (self->data_socket);
    zmq_close (self->ctrl_socket);

    free (self);
    *self_p = NULL;
}


//  --------------------------------------------------------------------------
//  Connect RPC client to server
//  RPC client can be connected to multiple servers

void
zfl_rpc_connect (zfl_rpc_t *self, char *server_id, char *endpoint)
{
    zfl_msg_t *msg = zfl_msg_new ();
    assert (msg);
    zfl_msg_push (msg, endpoint);
    zfl_msg_push (msg, server_id);
    zfl_msg_push (msg, "connect");
    zfl_msg_send (&msg, self->ctrl_socket);

    //  Receive and drop response
    msg = zfl_msg_recv (self->ctrl_socket);
    zfl_msg_destroy (&msg);
}


//  --------------------------------------------------------------------------
//  Make remote procedure call
//  Returns server's reply

zfl_msg_t *
zfl_rpc_send (zfl_rpc_t *self, zfl_msg_t **request_p)
{
    zfl_msg_send (request_p, self->data_socket);
    zfl_msg_t *reply = zfl_msg_recv (self->data_socket);
    return reply;
}
