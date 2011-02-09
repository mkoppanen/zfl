/*  =========================================================================
    zdevice.c

    Device launcher that will start a queue, forwarder, or streamer device
    from a configuration file or automagically from the command line.  The
    config file can be in text or JSON format, see:

     * http://rfc.zeromq.org/spec:3 for JSON format
     * http://rfc.zeromq.org/spec:4 for text format

    Command-line example:

        zdevice zmq_queue ipc://frontend ipc://backend

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
#include "../include/zfl_config.h"
#include "../include/zfl_config_json.h"
#include "../include/zfl_config_zpl.h"
#include "../include/zfl_device.h"


static void
    s_start_configured_device (char *filename);
static void
    s_start_automagic_device (char *type, char *frontend, char *backend);
static void
    s_parse_device_type (char *type, int *device, int *frontend, int *backend);


int
main (int argc, char *argv [])
{
    if (argc == 2)
        s_start_configured_device (argv [1]);
    else
    if (argc == 4)
        s_start_automagic_device (argv [1], argv [2], argv [3]);
    else {
        printf ("\nzdevice - start standard 0MQ device\n");
        printf ("\n");
        printf ("    Copyright (c) 1991-2011 iMatix Corporation and contributors\n");
        printf ("    Part of the ZeroMQ Function Library: http://zfl.zeromq.org\n");
        printf ("    License: LGPLv3+\n");
        printf ("\n");
        printf ("zdevice CONFIG | DEVICE FRONTEND BACKEND\n\n");
        printf ("CONFIG:\n    Config file[1] or '-' meaning stdin\n");
        printf ("DEVICE:\n    'zmq_queue', 'zmq_forwarder', or 'zmq_streamer'\n");
        printf ("FRONTEND:\n    Endpoint for device frontend socket\n");
        printf ("BACKEND:\n    Endpoint for device backend socket\n");
        printf ("\n");
        printf ("[1] see http://rfc.zeromq.org/spec:4, http://rfc.zeromq.org/spec:5\n");
    }
    return 0;
}


//  Starts a device as configured by a JSON or text config file
//
static void
s_start_configured_device (char *filename)
{


    //  Create and configure a zfl_device object
    zfl_device_t *device = zfl_device_new (filename);
    if (!device) {
        printf ("E: '%s' can't be read, or has invalid syntax\n", filename);
        exit (EXIT_FAILURE);
    }
    //  Find first device
    char *main_device = zfl_device_locate (device, 0);
    if (!*main_device) {
        printf ("E: No device specified, please read http://rfc.zeromq.org/spec:5\n");
        exit (EXIT_FAILURE);
    }
    //  Process device type
    char *type = zfl_device_property (device, main_device, "type");

    int device_type;            //  0MQ defined type
    int frontend_type;          //  Socket types depending
    int backend_type;           //    on the device type
    s_parse_device_type (type, &device_type, &frontend_type, &backend_type);

    //  Create and configure sockets
    void *frontend = zfl_device_socket (device, main_device, "frontend", frontend_type);
    assert (frontend);
    void *backend = zfl_device_socket (device, main_device, "backend", backend_type);
    assert (backend);

    //  Start the device now
    if (zfl_device_verbose (device))
        printf ("I: Starting device...\n");

    //  Will not actually ever return
    zmq_device (device_type, frontend, backend);

    zfl_device_destroy (&device);
}


//  Parse device type and set device/frontend/backend values for
//  calling zmq_device(3).
//
static void
s_parse_device_type (char *type, int *device, int *frontend, int *backend)
{
    if (streq (type, "zmq_queue")) {
        *device = ZMQ_QUEUE;
        *frontend = ZMQ_XREP;
        *backend = ZMQ_XREQ;
    }
    else
    if (streq (type, "zmq_forwarder")) {
        *device = ZMQ_FORWARDER;
        *frontend = ZMQ_SUB;
        *backend = ZMQ_PUB;
    }
    else
    if (streq (type, "zmq_streamer")) {
        *device = ZMQ_STREAMER;
        *frontend = ZMQ_PULL;
        *backend = ZMQ_PUSH;
    }
    else {
        printf ("E: Invalid device type \"%s\"\n", type);
        exit (EXIT_FAILURE);
    }
}


//  Starts an automagical device
//
//  - zmq_queue acts as broker, binds XREP and XREQ
//  - zmq_forwarder acts as proxy, connects SUB, binds PUB
//  - zmq_streamer acts as proxy, binds PULL, connects PUSH
//
static void
s_start_automagic_device (char *type, char *frontend, char *backend)
{
    void *context = zmq_init (1);

    int device;                 //  0MQ defined type
    int frontend_type;          //  Socket types depending
    int backend_type;           //    on the device type
    s_parse_device_type (type, &device, &frontend_type, &backend_type);

    //  Create and configure sockets
    void *frontend_socket = zmq_socket (context, frontend_type);
    assert (frontend_socket);
    void *backend_socket = zmq_socket (context, backend_type);
    assert (backend_socket);

    if (device == ZMQ_QUEUE) {
        printf ("I: Binding to %s for client connections\n", frontend);
        if (zmq_bind (frontend_socket, frontend)) {
            printf ("E: cannot bind to '%s': %s\n", frontend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
        printf ("I: Binding to %s for service connections\n", backend);
        if (zmq_bind (backend_socket, backend)) {
            printf ("E: cannot bind to '%s': %s\n", backend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
    }
    else
    if (device == ZMQ_FORWARDER) {
        printf ("I: Connecting to publisher at %s\n", frontend);
        if (zmq_connect (frontend_socket, frontend)) {
            printf ("E: cannot connect to '%s': %s\n", frontend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
        printf ("I: Binding to %s for subscriber connections\n", backend);
        if (zmq_bind (backend_socket, backend)) {
            printf ("E: cannot bind to '%s': %s\n", backend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
    }
    else
    if (device == ZMQ_STREAMER) {
        printf ("I: Binding to %s for upstream nodes\n", frontend);
        if (zmq_bind (frontend_socket, frontend)) {
            printf ("E: cannot bind to '%s': %s\n", frontend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
        printf ("I: Connecting downstream to %s\n", backend);
        if (zmq_connect (backend_socket, backend)) {
            printf ("E: cannot connect to '%s': %s\n", backend, zmq_strerror (errno));
            exit (EXIT_FAILURE);
        }
    }
    zmq_device (device, frontend_socket, backend_socket);
}
