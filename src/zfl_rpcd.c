/*  =========================================================================
    zfl_rpcd.c - server side reliable RPC

    Server side API for implementing reliable remote procedure calls.

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
#include "../include/zfl_rpcd.h"

//  How often we should check for heartbeat signal
#define HEARTBEAT_INTERVAL      1000000

//  Structure of our class

struct _zfl_rpcd {
    void
        *data_socket,   //  used to receive requests/send responses
        *ctrl_socket;   //  used to control RPC thread
    zfl_thread_t
        *thread;        //  handle to RPC thread
};

//  Internal structure used by RPC thread

typedef struct {
    void
        *frontend,      //  client requests and heartbeats
        *backend,       //  socket to send requests/receive response
        *control;       //  API uses control socket to communicate with RPC thread
    int
        server_busy;    //  indicates whether the server waits for response
    zfl_list_t
        *clients,       //  list of all connected clients
        *msg_queue;     //  queue of pending requests
    zfl_hash_t
        *registry;      //  used to lookup client using the ID
} rpcd_t;


//  Used to pass arguments on RPC thread creation

typedef struct {
    void
        *context;       // 0MQ context
    char
        *server_id,
        *data_endpoint,
        *ctrl_endpoint;
} thread_args_t;


//  Used to keep track of connected clients

struct client {
    char
        *client_id;     //  client ID
    uint64_t
        timestamp;      //  time we received the last request or heartbeat
};


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
#	error "zfl_rpc does not compile on this system"
#endif
}


//  --------------------------------------------------------------------------
//  Creates new client

static struct client *
s_client_new (char *id)
{
    struct client *client = (struct client *) zmalloc (sizeof (struct client));
    client->client_id = strdup (id);
    client->timestamp = s_now ();
    return client;
}


//  --------------------------------------------------------------------------
//  Deallocate client structure

static void
s_client_destroy (struct client **self_p)
{
    struct client *self = *self_p;
    free (self->client_id);
    free (self);
    *self_p = NULL;
}


//  --------------------------------------------------------------------------
//  Handle message from a client

static void
s_frontend_event (rpcd_t *rpcd)
{
    zfl_msg_t *msg = zfl_msg_recv (rpcd->frontend);
    assert (msg);
    assert (zfl_msg_parts (msg) > 0);

    char *client_id = zfl_msg_unwrap (msg);
    assert (client_id);

    struct client *client = (struct client *) zfl_hash_lookup (rpcd->registry, client_id);
    if (client == NULL) {
        client = s_client_new (client_id);
        assert (client);
        zfl_list_append (rpcd->clients, client);
        zfl_hash_insert (rpcd->registry, client->client_id, client);
    }
    if (zfl_msg_parts (msg) > 0) {
        //  Queue message
        zfl_msg_wrap (msg, client_id, NULL);
        zfl_list_append (rpcd->msg_queue, msg);
    }
    else {
        //  Echo heartbeat
        zfl_msg_wrap (msg, client_id, "");
        zfl_msg_send (&msg, rpcd->frontend);
    }
    client->timestamp = s_now ();
    zfl_list_remove (rpcd->clients, client);
    zfl_list_append (rpcd->clients, client);
    free (client_id);
}


//  --------------------------------------------------------------------------
//  Reply response from server to client

static void
s_backend_event (rpcd_t *rpcd)
{
    zfl_msg_t *msg = zfl_msg_recv (rpcd->backend);
    assert (msg);
    assert (rpcd->server_busy);
    zfl_msg_send (&msg, rpcd->frontend);
    rpcd->server_busy = 0;
}


//  --------------------------------------------------------------------------
//  Handle control request
//  Returns 1 when application asks for thread termination.

static int
s_control_event (rpcd_t *rpcd)
{
    int ret = 0;

    zfl_msg_t *req = zfl_msg_recv (rpcd->control);
    assert (req);
    assert (zfl_msg_parts (req) > 0);
    char *command = zfl_msg_pop (req);
    if (strcmp (command, "stop") == 0) {
        assert (zfl_msg_parts (req) == 0);
        ret = 1;
    }
    else {
        assert (strcmp (command, "bind") == 0);
        assert (zfl_msg_parts (req) == 1);
        char *endpoint = zfl_msg_pop (req);
        assert (endpoint);
        zmq_bind (rpcd->frontend, endpoint);
        free (endpoint);
    }

    zfl_msg_destroy (&req);
    free (command);

    return ret;
}


//  --------------------------------------------------------------------------
//  Main RPC client thread; this is what we talk to via other methods
//  It accepts requests from frontend socket and forwards them to
//  application thread. The design assumes the application thread can
//  process at most one request at a time. There is also a control
//  socket that provides a way for application thread to control
//  RPC thread.

static void *
s_rpcd_thread (void *arg)
{
    thread_args_t *thread_args = (thread_args_t *) arg;
    int rc;

    rpcd_t *rpcd = (rpcd_t *) zmalloc (sizeof (rpcd_t));

    //  Create frontend socket and sets its identity
    rpcd->frontend = zmq_socket (thread_args->context, ZMQ_XREP);
    assert (rpcd->frontend);
    rc = zmq_setsockopt (rpcd->frontend, ZMQ_IDENTITY,
        thread_args->server_id, strlen (thread_args->server_id));
    assert (rc == 0);

    //  Create backend socket and bind endpoint to it
    rpcd->backend = zmq_socket (thread_args->context, ZMQ_REQ);
    assert (rpcd->backend);
    rc = zmq_connect (rpcd->backend, thread_args->data_endpoint);
    assert (rc == 0);

    rpcd->control = zmq_socket (thread_args->context, ZMQ_PULL);
    assert (rpcd->control);
    rc = zmq_connect (rpcd->control, thread_args->ctrl_endpoint);
    assert (rc == 0);

    //  Free argument struct
    free (thread_args->server_id);
    free (thread_args->data_endpoint);
    free (thread_args->ctrl_endpoint);
    free (thread_args);

    //  No clients connected
    rpcd->clients = zfl_list_new ();
    assert (rpcd->clients);

    rpcd->registry = zfl_hash_new ();
    assert (rpcd->registry);

    //  No requests pending
    rpcd->msg_queue = zfl_list_new ();
    assert (rpcd->msg_queue);

    //  Controls thread termination.
    int stopped = 0;

    //  How long to wait while polling. The actual value
    //  is recalculated in the loop below.
    long poll_timeout = -1;

    while (!stopped) {
        zmq_pollitem_t items [] = {
            //  Wait for heartbeat signal and client requests
            {rpcd->frontend, -1, ZMQ_POLLIN, 0},
            //  Wait for response
            {rpcd->backend, -1, ZMQ_POLLIN, 0},
            //  Wait for control messages
            {rpcd->control, -1, ZMQ_POLLIN, 0}
        };

        rc = zmq_poll (items, 3, poll_timeout);
        assert (rc != -1);

        if (items [0].revents & ZMQ_POLLIN)
            //  Either request or heartbeat
            s_frontend_event (rpcd);
        if (items [1].revents & ZMQ_POLLIN)
            //  Response to last request
            s_backend_event (rpcd);
        if (items [2].revents & ZMQ_POLLIN)
            //  Stop message
            stopped = s_control_event (rpcd);

        //  get current time
        uint64_t now = s_now ();

        while (zfl_list_size (rpcd->clients) > 0) {
            struct client *client = (struct client *) zfl_list_first (rpcd->clients);
            assert (client);
            if (now < client->timestamp + HEARTBEAT_INTERVAL)
                break;
            zfl_list_remove (rpcd->clients, client);
            zfl_hash_delete (rpcd->registry, client->client_id);
            s_client_destroy (&client);
        }

        //  If there is a message in the message queue, forward
        //  it to the server
        if (zfl_list_size (rpcd->msg_queue) > 0) {
            if (!rpcd->server_busy) {
                zfl_msg_t *msg = (zfl_msg_t *) zfl_list_first (rpcd->msg_queue);
                zfl_list_remove (rpcd->msg_queue, msg);
                zfl_msg_send (&msg, rpcd->backend);
                rpcd->server_busy = 1;
            }
        }

        if (zfl_list_size (rpcd->clients) == 0)
            poll_timeout = -1;
        else {
            struct client *client = (struct client *) zfl_list_first (rpcd->clients);
            poll_timeout = (long) (now + HEARTBEAT_INTERVAL - client->timestamp);
        }
    }

    //  Close sockets
    zmq_close (rpcd->frontend);
    zmq_close (rpcd->backend);
    zmq_close (rpcd->control);

    //  Free all clients
    while (zfl_list_size (rpcd->clients)) {
        struct client *client = (struct client *) zfl_list_first (rpcd->clients);
        zfl_list_remove (rpcd->clients, client);
        s_client_destroy (&client);
    }

    //  Free all queued messages
    while (zfl_list_size (rpcd->msg_queue) > 0) {
        zfl_msg_t *msg = (zfl_msg_t *) zfl_list_first (rpcd->msg_queue);
        zfl_list_remove (rpcd->msg_queue, msg);
        zfl_msg_destroy (&msg);
    }

    //  Destroy data structures
    zfl_list_destroy (&rpcd->clients);
    zfl_hash_destroy (&rpcd->registry);
    zfl_list_destroy (&rpcd->msg_queue);

    free (rpcd);

    return NULL;
}


//  --------------------------------------------------------------------------
//  Constructor

zfl_rpcd_t *
zfl_rpcd_new (void *zmq_context, char *server_id)
{
    int rc;
    char
        data_endpoint [32],
        ctrl_endpoint [32];

    zfl_rpcd_t *self = (zfl_rpcd_t *) zmalloc (sizeof (zfl_rpcd_t));

    //  Prepare thread arguments
    self->data_socket = zmq_socket (zmq_context, ZMQ_REP);
    assert (self->data_socket);
    self->ctrl_socket = zmq_socket (zmq_context, ZMQ_PUSH);
    assert (self->ctrl_socket);

    //  Allow any number of RPC servers to exist in our process
	srandom ((unsigned) time (NULL));
    while (1) {
        long id = within (0x100000000);
        sprintf (data_endpoint, "inproc://rpcd/%08lX/data", id);
        sprintf (ctrl_endpoint, "inproc://rpcd/%08lX/ctrl", id);
        rc = zmq_bind (self->data_socket, data_endpoint);
        if (rc == 0)
            break;
    }
    rc = zmq_bind (self->ctrl_socket, ctrl_endpoint);
    assert (rc == 0);

    thread_args_t *thread_args = (thread_args_t *) zmalloc (sizeof (thread_args_t));
    thread_args->context = zmq_context;
    thread_args->server_id = strdup (server_id);
    thread_args->data_endpoint = strdup (data_endpoint);
    thread_args->ctrl_endpoint = strdup (ctrl_endpoint);

    self->thread = zfl_thread_new (s_rpcd_thread, thread_args);
    assert (self->thread);

    return self;
}


//  --------------------------------------------------------------------------
//  Destructor

void
zfl_rpcd_destroy (zfl_rpcd_t **self_p)
{
    zfl_rpcd_t *self = *self_p;
    int rc;

    if (!self)
        return;

    //  Send 'stop' request to the RPC thread
    zfl_msg_t *stop_msg = zfl_msg_new ();
    assert (stop_msg);
    zfl_msg_push (stop_msg, "stop");
    zfl_msg_send (&stop_msg, self->ctrl_socket);

    //  Wait until RPC thread terminates
    zfl_thread_wait (self->thread);
    zfl_thread_destroy (&self->thread);

    //  Close sockets
    rc = zmq_close (self->data_socket);
    assert (rc == 0);
    rc = zmq_close (self->ctrl_socket);
    assert (rc == 0);

    free (self);
    *self_p = NULL;
}


//  --------------------------------------------------------------------------
//  Creates endpoint and bind it to the server's socket.
//  Clients can connect to this endpoint and send their requests to the server

void
zfl_rpcd_bind (zfl_rpcd_t *self, char *endpoint)
{
    if (!self)
        return;

    zfl_msg_t *bind_req = zfl_msg_new ();
    assert (bind_req);
    zfl_msg_push (bind_req, endpoint);
    zfl_msg_push (bind_req, "bind");
    zfl_msg_send (&bind_req, self->ctrl_socket);
}


//  --------------------------------------------------------------------------
//  Receive request from the RPC server thread
//  Blocks if the message is not ready
//  The caller is responsible for destroying the returned message

zfl_msg_t *
zfl_rpcd_recv (zfl_rpcd_t *self)
{
    if (self)
        return zfl_msg_recv (self->data_socket);
    return NULL;
}


//  --------------------------------------------------------------------------
//  Send response to the RPC server thread

void
zfl_rpcd_send (zfl_rpcd_t *self, zfl_msg_t **msg_p)
{
    if (self)
        zfl_msg_send (msg_p, self->data_socket);
}


//  --------------------------------------------------------------------------
//  Selftest

int
zfl_rpcd_test (Bool verbose)
{
    zfl_rpcd_t
        *rpcd;

    printf (" * zfl_rpcd: ");

    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    if ((major * 1000 + minor * 100 + patch) < 2100) {
        printf ("E: need at least 0MQ version 2.1.0\n");
        exit (EXIT_FAILURE);
    }
    void *context = zmq_init (1);
    assert (context);

    rpcd = zfl_rpcd_new (context, "master");
    assert (rpcd);
    zfl_rpcd_bind (rpcd, "tcp://*:5001");

    //  Don't actually wait for input since the client won't be there

    zfl_rpcd_destroy (&rpcd);
    assert (rpcd == NULL);

    zmq_term (context);
    printf ("OK\n");
    return 0;
}
