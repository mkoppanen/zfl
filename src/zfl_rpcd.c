/*  =========================================================================
    zfl_rpcd.c - server side RPC

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
#include "../include/zfl_rpcd.h"

//  How often we should check for heartbeat signal
#define HEARTBEAT_INTERVAL      1000000

//  Structure of our class

struct _zfl_rpcd {
    void
        *rpc_socket,    //  used to receive requests/send responses
        *control_socket;//  used to RPC thread control
    pthread_t
        thread;         //  handle to RPC thread
};

//  RPC server state

struct rpc_server {
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
};


//  Used to pass arguments on RPC thread creation

struct server_args {
    void
        *context;       // 0MQ context
    char
        *server_id,
        *endpoint,
        *control_endpoint;
};


//  Used to keep track of connected clients

struct client {
    char
        *client_id;     //  client ID
    uint64_t
        timestamp;      //  time we received the last request or heartbeat
};


//  --------------------------------------------------------------------------
//  Return current time in us

static uint64_t
now_us ()
{
    struct timeval tv;
    int rc = gettimeofday (&tv, NULL);
    assert (rc == 0);
    return (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}


//  --------------------------------------------------------------------------
//  Creates new client

static struct client *
client_new(char *id)
{
    struct client *client = zmalloc (sizeof (struct client));
    assert (client);
    client->client_id = strdup (id);
    client->timestamp = now_us ();
    return client;
}


//  --------------------------------------------------------------------------
//  Deallocate client structure

static void
client_destroy (struct client **self_p)
{
    struct client *self = *self_p;
    free (self->client_id);
    free (self);
    *self_p = NULL;
}


//  --------------------------------------------------------------------------
//  Handle message from a client

static void
frontend_event (struct rpc_server *rpc)
{
    zfl_msg_t *msg = zfl_msg_recv (rpc->frontend);
    assert (msg);
    assert (zfl_msg_parts (msg) > 0);

    char *client_id = zfl_msg_unwrap (msg);
    assert (client_id);

    struct client *client = zfl_hash_lookup (rpc->registry, client_id);
    if (client == NULL) {
        client = client_new (client_id);
        assert (client);
        zfl_list_append (rpc->clients, client);
        zfl_hash_insert (rpc->registry, client->client_id, client);
    }
    if (zfl_msg_parts (msg) > 0) {
        //  Queue message
        zfl_msg_wrap (msg, client_id, NULL);
        zfl_list_append (rpc->msg_queue, msg);
    }
    else {
        //  Echo heartbeat
        zfl_msg_wrap (msg, client_id, "");
        zfl_msg_send (&msg, rpc->frontend);
    }

    client->timestamp = now_us ();
    zfl_list_remove (rpc->clients, client);
    zfl_list_append (rpc->clients, client);
}


//  --------------------------------------------------------------------------
//  Rely response from server to client

static void
backend_event (struct rpc_server *rpc)
{
    zfl_msg_t *msg = zfl_msg_recv (rpc->backend);
    assert (msg);
    assert (rpc->server_busy);
    zfl_msg_send (&msg, rpc->frontend);
    rpc->server_busy = 0;
}


//  --------------------------------------------------------------------------
//  Handle control request
//  Returns 1 when application asks for thread termination.

static int
control_event (struct rpc_server *rpc)
{
    int ret = 0;

    zfl_msg_t *req = zfl_msg_recv (rpc->control);
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
        zmq_bind (rpc->frontend, endpoint);
        free (endpoint);
    }

    zfl_msg_destroy (&req);
    free (command);

    return ret;
}


//  RPC server thread.
//  It accepts requests from frontend socket and forwards them to
//  application thread. The design assumes the application thread can
//  process at most one request at a time. There is also a control
//  socket that provides a way for application thread to control
//  RPC thread.

static void *
server_thread (void *arg)
{
    struct server_args *server_args = arg;
    int rc;

    struct rpc_server *rpc = zmalloc (sizeof (struct rpc_server));
    assert (rpc);

    //  Create frontend socket and sets it identity
    rpc->frontend = zmq_socket (server_args->context, ZMQ_XREP);
    assert (rpc->frontend);
    rc = zmq_setsockopt (rpc->frontend, ZMQ_IDENTITY,
        server_args->server_id, strlen (server_args->server_id));
    assert (rc == 0);

    //  Create backend socket and bind endpoint to it
    rpc->backend = zmq_socket (server_args->context, ZMQ_REQ);
    assert (rpc->backend);
    rc = zmq_connect (rpc->backend, server_args->endpoint);
    assert (rc == 0);

    rpc->control = zmq_socket (server_args->context, ZMQ_PULL);
    assert (rpc->control);
    rc = zmq_connect (rpc->control, server_args->control_endpoint);
    assert (rc == 0);

    //  Free argument struct
    free (server_args->server_id);
    free (server_args->endpoint);
    free (server_args->control_endpoint);
    free (server_args);

    //  No clients connected
    rpc->clients = zfl_list_new ();
    assert (rpc->clients);

    rpc->registry = zfl_hash_new ();
    assert (rpc->registry);

    //  No requests pending
    rpc->msg_queue = zfl_list_new ();
    assert (rpc->msg_queue);

    //  Controls thread termination.
    int stop = 0;

    //  How long to wait while polling. The actual value
    //  is recalculated in the loop below.
    long poll_timeout = -1;

    while (!stop) {
        zmq_pollitem_t items [] = {
            //  Wait for heartbeat signal and client requests
            {rpc->frontend, -1, ZMQ_POLLIN, 0},
            //  Wait for response
            {rpc->backend, -1, ZMQ_POLLIN, 0},
            //  Wait for control messages
            {rpc->control, -1, ZMQ_POLLIN, 0}
        };

        rc = zmq_poll (items, 3, poll_timeout);
        assert (rc != -1);

        if (items [0].revents & ZMQ_POLLIN)
            //  Either request or heartbeat
            frontend_event (rpc);
        if (items [1].revents & ZMQ_POLLIN)
            //  Response to last request
            backend_event (rpc);
        if (items [2].revents & ZMQ_POLLIN)
            //  Stop message
            stop = control_event (rpc);

        //  get current time
        uint64_t now = now_us ();

        while (zfl_list_size (rpc->clients) > 0) {
            struct client *client = zfl_list_first (rpc->clients);
            assert (client);
            if (now < client->timestamp + HEARTBEAT_INTERVAL)
                break;
            zfl_list_remove (rpc->clients, client);
            zfl_hash_delete (rpc->registry, client->client_id);
            client_destroy (&client);
        }

        //  If there is a message in the message queue, forward
        //  it to the server
        if (zfl_list_size (rpc->msg_queue) > 0) {
            if (!rpc->server_busy) {
                zfl_msg_t *msg = zfl_list_first (rpc->msg_queue);
                zfl_list_remove (rpc->msg_queue, msg);
                zfl_msg_send (&msg, rpc->backend);
                rpc->server_busy = 1;
            }
        }

        if (zfl_list_size (rpc->clients) == 0)
            poll_timeout = -1;
        else {
            struct client *client = zfl_list_first (rpc->clients);
            poll_timeout = now + HEARTBEAT_INTERVAL - client->timestamp;
        }
    }

    //  Close sockets
    zmq_close (rpc->frontend);
    zmq_close (rpc->backend);
    zmq_close (rpc->control);

    //  Free all clients
    while (zfl_list_size (rpc->clients)) {
        struct client *client = zfl_list_first (rpc->clients);
        zfl_list_remove (rpc->clients, client);
        client_destroy (&client);
    }

    //  Free all queued messages
    while (zfl_list_size (rpc->msg_queue) > 0) {
        zfl_msg_t *msg = zfl_list_first (rpc->msg_queue);
        zfl_list_remove (rpc->msg_queue, msg);
        zfl_msg_destroy (&msg);
    }

    zfl_list_destroy (&rpc->clients);
    zfl_hash_destroy (&rpc->registry);
    zfl_list_destroy (&rpc->msg_queue);

    return NULL;
}


//  --------------------------------------------------------------------------
//  Formats endpoint for control socket

static char *
format_control_endpoint (char *endpoint)
{
    char *suffix = "-control";
    char *buf = malloc (strlen (endpoint) + strlen (suffix) + 1);
    assert (buf);
    strcpy (buf, endpoint);
    strcat (buf, suffix);
    return buf;
}


//  --------------------------------------------------------------------------
//  Constructor

zfl_rpcd_t *
zfl_rpcd_new (void *zmq_context, char *server_id, char *endpoint)
{
    int rc;

    zfl_rpcd_t *self = zmalloc (sizeof (zfl_rpcd_t));
    assert (self);

    self->rpc_socket = zmq_socket (zmq_context, ZMQ_REP);
    assert (self->rpc_socket);
    rc = zmq_bind (self->rpc_socket, endpoint);
    assert (rc == 0);

    self->control_socket = zmq_socket (zmq_context, ZMQ_PUSH);
    assert (self->control_socket);
    char *control_endpoint = format_control_endpoint (endpoint);
    rc = zmq_bind (self->control_socket, control_endpoint);
    assert (rc == 0);

    //  Prepare thread arguments
    struct server_args *args = zmalloc (sizeof (struct server_args));
    assert (args);
    args->context = zmq_context;
    args->server_id = strdup (server_id);
    args->endpoint = strdup (endpoint);
    args->control_endpoint = control_endpoint;

    rc = pthread_create (&self->thread, NULL, server_thread, args);
    assert (rc == 0);

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
    zfl_msg_send (&stop_msg, self->control_socket);

    //  Wait until RPC thread terminates
    rc = pthread_join (self->thread, NULL);
    assert (rc == 0);

    //  Close sockets
    rc = zmq_close (self->rpc_socket);
    assert (rc == 0);
    rc = zmq_close (self->control_socket);
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
    zfl_msg_send (&bind_req, self->control_socket);
}


//  --------------------------------------------------------------------------
//  Receive request from the RPC server thread
//  Blocks if the message is not ready
//  The caller is responsible for destroying the returned message

zfl_msg_t *
zfl_rpcd_recv (zfl_rpcd_t *self)
{
    if (self)
        return zfl_msg_recv (self->rpc_socket);
    return NULL;
}


//  --------------------------------------------------------------------------
//  Send response to the RPC server thread

void
zfl_rpcd_send (zfl_rpcd_t *self, zfl_msg_t **msg_p)
{
    if (self)
        zfl_msg_send (msg_p, self->rpc_socket);
}
