/*  =========================================================================
    zfl_config.c

    Creates a 0MQ context and sockets, configuring them as specified by a
    zfl_tree class (which is usually loaded from a config file in various
    formats including text and JSON).

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
#include "../include/zfl_blob.h"
#include "../include/zfl_tree.h"
#include "../include/zfl_tree_zpl.h"
#include "../include/zfl_config.h"

//  Structure of our class

struct _zfl_config_t {
    zfl_tree_t
        *tree;              //  Property tree
    void
        *context;           //  0MQ context for our process
    int
        iothreads;          //  Number of I/O threads in context
    Bool
        verbose;            //  Show configuration in progress?
};


//  --------------------------------------------------------------------------
//  Constructor
//
//  Creates a config object containing a 0MQ context
//
zfl_config_t *
zfl_config_new (zfl_tree_t *tree)
{
    zfl_config_t
        *self = NULL;           //  New config object

    assert (tree);
    assert (self = zmalloc (sizeof (zfl_config_t)));
    self->tree = tree;

    self->verbose = atoi (zfl_tree_resolve (tree, "context/verbose", "0"));
    if (self->verbose)
        printf ("I: Configuration in progress\n");

    self->iothreads = atoi (zfl_tree_resolve (tree, "context/iothreads", "1"));
    if (self->iothreads < 1 || self->iothreads > 255) {
        printf ("W: ignoring illegal iothreads value %d\n", self->iothreads);
        self->iothreads = 1;
    }
    //  Initialize 0MQ as requested
    self->context = zmq_init (self->iothreads);
    return self;
}


//  --------------------------------------------------------------------------
//  Creates new config object from ZPL (text properties) file
//  Returns NULL if the file does not exist
//
zfl_config_t *
zfl_config_load (char *filename)
{
    zfl_config_t *self = NULL;
    zfl_tree_t *tree = zfl_tree_zpl_file (filename);
    if (tree) {
        self = zfl_config_new (tree);
    }
    return (self);
}


//  --------------------------------------------------------------------------
//  Destructor
//  Note, shuts down 0MQ
//
void
zfl_config_destroy (zfl_config_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_config_t *self = *self_p;
        zfl_tree_destroy (&self->tree);
        zmq_term (self->context);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Returns the name of the Nth service in the configuration tree. The first
//  service is index 0, following C conventions for arrays.  Returns NULL if
//  there is no such service.
//
char *
zfl_config_service (zfl_config_t *self, int index)
{
    assert (self);

    //  Check children of root item, skip any called "context"
    zfl_tree_t *tree = zfl_tree_child (self->tree);
    while (tree) {
        if (strneq (zfl_tree_name (tree), "context")) {
            if (index)
                index--;
            else
                return zfl_tree_name (tree);
        }
        tree = zfl_tree_next (tree);
    }
    return NULL;
}


//  --------------------------------------------------------------------------
//  Returns a named service property, or "" if not found.  Property can be a
//  path of names separted by '/', meaning resolve through multiple levels
//  starting from children of service.
//
char *
zfl_config_property (zfl_config_t *self, char *service_name, char *property)
{
    assert (self);
    assert (service_name);
    assert (property);

    zfl_tree_t *tree = zfl_tree_locate (self->tree, service_name);
    if (!tree)
        return "";              //  No such service
    return zfl_tree_resolve (tree, property, "");
}


//  Process options settings
//
int
s_setsockopt (zfl_config_t *self, void *socket, zfl_tree_t *tree)
{
    int rc = 0;
    tree = zfl_tree_child (tree);
    while (tree && rc == 0) {
        char *name = zfl_tree_name (tree);
        char *value = zfl_tree_string (tree);
        if (streq (name, "hwm")) {
            uint64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_HWM, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "swap")) {
            int64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_SWAP, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "affinity")) {
            uint64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_AFFINITY, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "identity"))
            rc = zmq_setsockopt (socket, ZMQ_IDENTITY, value, strlen (value));
        else
        if (streq (name, "subscribe"))
            rc = zmq_setsockopt (socket, ZMQ_SUBSCRIBE, value, strlen (value));
        else
        if (streq (name, "rate")) {
            int64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_RATE, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "recovery_ivl")) {
            int64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_RECOVERY_IVL, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "mcast_loop")) {
            int64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_MCAST_LOOP, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "sndbuf")) {
            uint64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_SNDBUF, &optvalue, sizeof (optvalue));
        }
        else
        if (streq (name, "rcvbuf")) {
            uint64_t optvalue = atoi (value);
            rc = zmq_setsockopt (socket, ZMQ_RCVBUF, &optvalue, sizeof (optvalue));
        }
        else
        if (self->verbose)
            printf ("W: ignoring socket option '%s'\n", name);

        tree = zfl_tree_next (tree);
    }
    return rc;
}

//  --------------------------------------------------------------------------
//  Creates a named 0MQ socket within a named service, and configures the
//  socket as specified in the configuration data.  Returns NULL if the
//  service or socket do not exist, or if there was an error configuring the
//  socket.
//
void *
zfl_config_socket (
    zfl_config_t *self,
    char *service_name,
    char *socket_name,
    int   socket_type)
{
    assert (self);
    assert (service_name);
    assert (strneq (service_name, "context"));

    zfl_tree_t *tree = zfl_tree_locate (self->tree, service_name);
    if (!tree)
        return NULL;            //  No such service

    void *socket = zmq_socket (self->context, socket_type);
    if (!socket)
        return NULL;            //  Can't create socket

    if (zfl_config_verbose (self))
        printf ("I: Configuring '%s' socket in '%s' service...\n",
            socket_name, service_name);

    //  Find socket in service
    int rc = 0;
    tree = zfl_tree_locate (tree, socket_name);
    if (tree) {
        tree = zfl_tree_child (tree);
        while (tree && rc == 0) {
            char *name = zfl_tree_name (tree);
            if (streq (name, "bind"))
                rc = zmq_bind (socket, zfl_tree_string (tree));
            else
            if (streq (name, "connect"))
                rc = zmq_connect (socket, zfl_tree_string (tree));
            else
            if (streq (name, "option"))
                rc = s_setsockopt (self, socket, tree);
            //
            //  else ignore it, user space setting

            tree = zfl_tree_next (tree);
        }
    }
    else
    if (self->verbose)
        printf ("W: No configuration found for '%s'\n", socket_name);

    if (rc) {
        printf ("E: configuration failed - %s\n", zmq_strerror (errno));
        zmq_close (socket);
        socket = NULL;
    }
    return socket;
}


//  --------------------------------------------------------------------------
//  Returns the 0MQ context associated with this config
//
void *
zfl_config_context (zfl_config_t *self)
{
    assert (self);
    return self->context;
}


//  --------------------------------------------------------------------------
//  Returns the verbose property
//
Bool
zfl_config_verbose (zfl_config_t *self)
{
    assert (self);
    return self->verbose;
}


//  --------------------------------------------------------------------------
//  Selftest

int
zfl_config_test (Bool verbose)
{
    printf (" * zfl_config: ");

    //  Create a new config from the ZPL test file
    zfl_config_t *config = zfl_config_load ("zfl_config_test.txt");
    assert (config);

    //  Test unknown service
    void *socket = zfl_config_socket (config, "nosuch", "socket", ZMQ_SUB);
    assert (socket == NULL);
    zmq_close (socket);

    //  Find real service
    char *service = zfl_config_service (config, 0);
    assert (*service);
    assert (streq (service, "main"));

    char *type = zfl_config_property (config, service, "type");
    assert (*type);
    assert (streq (type, "zqueue"));

    char *endpoint = zfl_config_property (config, service, "frontend/endpoint");
    assert (*endpoint);
    assert (streq (endpoint, "valid-endpoint"));

    //  Configure two sockets
    void *frontend = zfl_config_socket (config, service, "frontend", ZMQ_SUB);
    assert (frontend);
    zmq_close (frontend);

    void *backend = zfl_config_socket (config, service, "backend", ZMQ_PUB);
    assert (backend);
    zmq_close (backend);

    zfl_config_destroy (&config);
    assert (config == NULL);
    printf ("OK\n");
    return 0;
}
