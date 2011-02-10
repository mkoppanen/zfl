/*  =========================================================================
    zfl_device.c

    Used to configure 0MQ devices and their sockets. Takes configuration data
    from a zfl_config object, and implements the rfc.zeromq.org/spec:5/zdcf
    specification. Use this class to for stand-alone devices. Do not use for
    built-in devices (i.e. which operate as threads of larger processes). See
    examples/zdevice.c for a working example.

    TODO:
    - track open sockets and force them closed before calling zmq_term

    -------------------------------------------------------------------------
    Copyright (c) 1991-2011 iMatix Corporation <www.imatix.com>
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
#include "../include/zfl_config.h"
#include "../include/zfl_device.h"

//  Structure of our class

struct _zfl_device_t {
    zfl_config_t
        *config;            //  Property config
    void
        *context;           //  0MQ context for our process
    int
        iothreads;          //  Number of I/O threads in context
    Bool
        verbose;            //  Show property in progress?
};

//  Local functions
static int
    s_setsockopt (zfl_device_t *self, void *socket, zfl_config_t *config);


//  --------------------------------------------------------------------------
//  Constructor
//
//  Creates a device object from a JSON or ZPL config file that follows the
//  ZDCF specification. If the file looks like JSON it's parsed as such, else
//  it's parsed as ZPL. If the filename is "-", reads from STDIN. Creates a
//  0MQ context for the device, initialized as specified in the config file.
//  Returns NULL if the file does not exist or cannot be read.

zfl_device_t *
zfl_device_new (char *filename)
{
    zfl_config_t *config = zfl_config_load (filename);
    if (config == NULL)
        return NULL;                //  Syntax error in file data

    //  Now create the device object and configure its context
    zfl_device_t *self = NULL;
    self = (zfl_device_t *) zmalloc (sizeof (zfl_device_t));
    self->config = config;          //  Device object owns the config

    self->verbose = atoi (zfl_config_resolve (config, "context/verbose", "0"));
    if (self->verbose)
        printf ("I: Configuration in progress\n");

    self->iothreads = atoi (zfl_config_resolve (config, "context/iothreads", "1"));
    if (self->iothreads < 1 || self->iothreads > 255) {
        printf ("W: ignoring illegal iothreads value %d\n", self->iothreads);
        self->iothreads = 1;
    }
    //  Initialize 0MQ as requested
    self->context = zmq_init (self->iothreads);

    //  Return configured device object
    return self;
}


//  --------------------------------------------------------------------------
//  Destructor
//  Note, shuts down 0MQ

void
zfl_device_destroy (zfl_device_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_device_t *self = *self_p;
        zfl_config_destroy (&self->config);
        zmq_term (self->context);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Returns the 0MQ context associated with this device

void *
zfl_device_context (zfl_device_t *self)
{
    assert (self);
    return self->context;
}


//  --------------------------------------------------------------------------
//  Returns the verbose property

Bool
zfl_device_verbose (zfl_device_t *self)
{
    assert (self);
    return self->verbose;
}


//  --------------------------------------------------------------------------
//  Returns the name of the Nth device in the property config. The first
//  device following the context specification has index 0, following C
//  conventions for arrays. Returns NULL if there is no such device.
//
char *
zfl_device_locate (zfl_device_t *self, int index)
{
    assert (self);

    //  Check children of root item, skip any called "context"
    zfl_config_t *config = zfl_config_child (self->config);
    while (config) {
        if (strneq (zfl_config_name (config), "context")) {
            if (index)
                index--;
            else
                return zfl_config_name (config);
        }
        config = zfl_config_next (config);
    }
    return NULL;
}


//  --------------------------------------------------------------------------
//  Returns a named device property, or "" if not found. Property can be a
//  path of names separted by '/', meaning resolve through multiple levels
//  starting from children of device.

char *
zfl_device_property (zfl_device_t *self, char *device_name, char *property)
{
    assert (self);
    assert (device_name);
    assert (strneq (device_name, "context"));
    assert (property);

    zfl_config_t *config = zfl_config_locate (self->config, device_name);
    if (!config)
        return "";              //  No such device
    return zfl_config_resolve (config, property, "");
}


//  --------------------------------------------------------------------------
//  Creates a named 0MQ socket within a named device, and deviceures the
//  socket as specified in the property data. Returns NULL if the
//  device or socket do not exist, or if there was an error deviceuring the
//  socket.

void *
zfl_device_socket (
    zfl_device_t *self,
    char *device_name,
    char *socket_name,
    int   socket_type)
{
    assert (self);
    assert (device_name);
    assert (strneq (device_name, "context"));

    zfl_config_t *config = zfl_config_locate (self->config, device_name);
    if (!config)
        return NULL;            //  No such device

    void *socket = zmq_socket (self->context, socket_type);
    if (!socket)
        return NULL;            //  Can't create socket

    if (zfl_device_verbose (self))
        printf ("I: Configuring '%s' socket in '%s' device...\n",
            socket_name, device_name);

    //  Find socket in device
    int rc = 0;
    config = zfl_config_locate (config, socket_name);
    if (config) {
        config = zfl_config_child (config);
        while (config && rc == 0) {
            char *name = zfl_config_name (config);
            if (streq (name, "bind"))
                rc = zmq_bind (socket, zfl_config_string (config));
            else
            if (streq (name, "connect"))
                rc = zmq_connect (socket, zfl_config_string (config));
            else
            if (streq (name, "option"))
                rc = s_setsockopt (self, socket, config);
            //
            //  else ignore it, user space setting

            config = zfl_config_next (config);
        }
    }
    else
    if (self->verbose)
        printf ("W: No property found for '%s'\n", socket_name);

    if (rc) {
        printf ("E: property failed - %s\n", zmq_strerror (errno));
        zmq_close (socket);
        socket = NULL;
    }
    return socket;
}

//  Process options settings
//
int
s_setsockopt (zfl_device_t *self, void *socket, zfl_config_t *config)
{
    int rc = 0;
    config = zfl_config_child (config);
    while (config && rc == 0) {
        char *name = zfl_config_name (config);
        char *value = zfl_config_string (config);
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

        config = zfl_config_next (config);
    }
    return rc;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zfl_device_test (Bool verbose)
{
    printf (" * zfl_device: ");

    //  Create a new device from the ZPL test file
    zfl_device_t *device = zfl_device_new ("zfl_device_test.txt");
    assert (device);

    //  Look for device that does not exist
    void *socket = zfl_device_socket (device, "nosuch", "socket", ZMQ_SUB);
    assert (socket == NULL);
    zmq_close (socket);

    //  Look for existing device, configure it
    char *main_device = zfl_device_locate (device, 0);
    assert (*main_device);
    assert (streq (main_device, "main"));

    char *type = zfl_device_property (device, main_device, "type");
    assert (*type);
    assert (streq (type, "zmq_queue"));

    char *endpoint = zfl_device_property (device, main_device, "frontend/endpoint");
    assert (*endpoint);
    assert (streq (endpoint, "valid-endpoint"));

    //  Configure two sockets
    void *frontend = zfl_device_socket (device, main_device, "frontend", ZMQ_SUB);
    assert (frontend);
    zmq_close (frontend);

    void *backend = zfl_device_socket (device, main_device, "backend", ZMQ_PUB);
    assert (backend);
    zmq_close (backend);

    zfl_device_destroy (&device);
    assert (device == NULL);
    printf ("OK\n");
    return 0;
}
