/*  =========================================================================
    zfl_config.c

    Creates a 0MQ context and sockets, configuring them as specified by a
    JSON configuration file.  The config file follows the format described
    at http://rfc.zeromq.org/spec:3.

    Does not provide detailed error reporting.  To verify your JSON files
    use http://www.jsonlint.com.


    Copyright (c) 1991-2010 iMatix Corporation and contributors

    This file is part of the ZeroMQ Function Library: http://zfl.zeromq.org

    This is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include <zmq.h>
#include "../include/zfl_prelude.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_config.h"

//  Import the cJSON library
#include "import/cJSON/cJSON.h"
#include "import/cJSON/cJSON.c"

//  Structure of our class

struct _zfl_config_t {
    cJSON
        *json;              //  JSON tree
    void
        *context;           //  0MQ context for our process
    int
        iothreads;          //  Number of I/O threads in context
    Bool
        verbose;            //  Show configuration in progress?
};

//  Handlers for socket configuration; each returns zero on success
//  or -1 on failure.
//
static int s_do_bind (void *socket, char *strvalue, int intvalue)
{
    return (zmq_bind (socket, strvalue));
}
static int s_do_connect (void *socket, char *strvalue, int intvalue)
{
    return (zmq_connect (socket, strvalue));
}
static int s_do_option_hwm (void *socket, char *strvalue, int intvalue)
{
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_HWM, &optvalue, sizeof (optvalue)));
}
static int s_do_option_swap (void *socket, char *strvalue, int intvalue)
{
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_SWAP, &optvalue, sizeof (optvalue)));
}
static int s_do_option_affinity (void *socket, char *strvalue, int intvalue)
{
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_AFFINITY, &optvalue, sizeof (optvalue)));
}
static int s_do_option_identity (void *socket, char *strvalue, int intvalue)
{
    return (zmq_setsockopt (socket, ZMQ_IDENTITY, strvalue, strlen (strvalue)));
}
static int s_do_option_subscribe (void *socket, char *strvalue, int intvalue)
{
    return (zmq_setsockopt (socket, ZMQ_SUBSCRIBE, strvalue, strlen (strvalue)));
}
static int s_do_option_rate (void *socket, char *strvalue, int intvalue)
{
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_RATE, &optvalue, sizeof (optvalue)));
}
static int s_do_option_recovery_ivl (void *socket, char *strvalue, int intvalue)
{
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_RECOVERY_IVL, &optvalue, sizeof (optvalue)));
}
static int s_do_option_mcast_loop (void *socket, char *strvalue, int intvalue)
{
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_MCAST_LOOP, &optvalue, sizeof (optvalue)));
}
static int s_do_option_sndbuf (void *socket, char *strvalue, int intvalue)
{
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_SNDBUF, &optvalue, sizeof (optvalue)));
}
static int s_do_option_rcvbuf (void *socket, char *strvalue, int intvalue)
{
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_RCVBUF, &optvalue, sizeof (optvalue)));
}

typedef int (helper_fct) (void *socket, char *strvalue, int intvalue);

static struct {
    char *path;
    helper_fct *helper;
} s_helpers [] = {
    { "bind", s_do_bind },
    { "connect", s_do_connect },
    { "option/hwm", s_do_option_hwm },
    { "option/swap", s_do_option_swap },
    { "option/affinity", s_do_option_affinity },
    { "option/identity", s_do_option_identity },
    { "option/subscribe", s_do_option_subscribe },
    { "option/rate", s_do_option_rate },
    { "option/recovery_ivl", s_do_option_recovery_ivl },
    { "option/mcast_loop", s_do_option_mcast_loop },
    { "option/sndbuf", s_do_option_sndbuf },
    { "option/rcvbuf", s_do_option_rcvbuf },
    //  Sentinel marks end of list
    { NULL, NULL }
};


//  Private function that processes one socket config in the tree.
//  Recurses down the tree calculating the full path and executes
//  leafs that match known paths.  To make this easy to extend, we
//  register all valid paths along with their handlers in a lookup
//  table.  That's probably overkill but it separates the generic
//  code from the 0MQ specific stuff and makes it easy to debug.
//
static void
s_parse_socket_item (
    zfl_config_t *self,         //  Config object
    void    *socket,            //  Socket to configure
    cJSON   *item,              //  JSON item to parse
    char    *path)              //  Path of this item
{
    assert (item);
    if (item->type == cJSON_Array) {
        item = item->child;
        while (item) {
            s_parse_socket_item (self, socket, item, path);
            item = item->next;
        }
    }
    else
    if (item->type == cJSON_Object) {
        char subpath [255 + 1];
        cJSON *child = item->child;
        while (child) {
            snprintf (subpath, 255,
                "%s%s%s", path, *path? "/":"", child->string);
            s_parse_socket_item (self, socket, child, subpath);
            child = child->next;
        }
    }
    else {
        //  Process elementary item
        int
            index;
        if (self->verbose) {
            if (item->type == cJSON_String)
                printf ("I: - %s = %s\n", path, item->valuestring);
            else
                printf ("I: - %s = %d\n", path, item->valueint);
        }
        for (index = 0;; index++) {
            if (!s_helpers [index].path) {
                if (self->verbose)
                    printf ("W: Unknown path '%s'\n", path);
                break;          //  Unknown path, ignore
            }
            if (streq (s_helpers [index].path, path)) {
                if ((s_helpers [index].helper)
                    (socket, item->valuestring, item->valueint))
                    printf ("E: could not do '%s': %s\n",
                        path, zmq_strerror (errno));
                break;          //  Path found, processed
            }
        }
    }
}


//  --------------------------------------------------------------------------
//  Constructor
//
//  Creates a config object containing a 0MQ context.
//  If the specified blob contains valid ZDCF data, loads it.
//
zfl_config_t *
zfl_config_new (char *data)
{
    zfl_config_t
        *self = NULL;           //  New config object
    cJSON
        *context,               //  Context object if any
        *item;                  //  Property of context

    assert (self = malloc (sizeof (zfl_config_t)));
    self->iothreads = 1;
    self->verbose = 0;

    //  Load JSON data and if it's NFG, load empty object
    self->json = cJSON_Parse (data);
    if (!self->json)
        cJSON_Parse ("{ }");

    //  Parse context object
    context = cJSON_GetObjectItem (self->json, "context");
    if (context) {
        item = context->child;
        while (item) {
            if (streq (item->string, "iothreads")) {
                if (item->valueint > 0)
                    self->iothreads = item->valueint;
                else
                    printf ("W: ignoring illegal iothreads value %d\n", item->valueint);
                if (self->verbose)
                    printf ("I: - will use %d I/O thread%s\n",
                        self->iothreads, self->iothreads > 1? "s":"");
            }
            else
            if (streq (item->string, "verbose")) {
                self->verbose = item->valueint;
                if (self->verbose)
                    printf ("I: Configuration in progress\n");
            }
            item = item->next;
        }
    }
    self->context = zmq_init (self->iothreads);
    return (self);
}


//  --------------------------------------------------------------------------
//  Destructor
//
void
zfl_config_destroy (zfl_config_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        cJSON_Delete ((*self_p)->json);
        free (*self_p);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Returns the name of the Nth device in the configuration tree.  The first
//  device is index 0, following C conventions for arrays.  Returns NULL if
//  there is no such device.
//
char *
zfl_config_device (zfl_config_t *self, int index)
{
    cJSON
        *item;                  //  Item in JSON tree

    assert (self);
    item = self->json->child;
    while (item) {
        if (strneq (item->string, "context")) {
            if (index)
                index--;
            else
                return (item->string);
        }
        item = item->next;
    }
    return (NULL);
}


//  --------------------------------------------------------------------------
//  Returns the type of the specified device, or "" if not specified.
//
char *
zfl_config_device_type (zfl_config_t *self, char *device)
{
    cJSON
        *item;

    assert (self);
    assert (device);
    assert (strneq (device, "context"));

    //  Find named device
    item = self->json->child;
    while (item) {
        if (streq (item->string, device)) {
            item = cJSON_GetObjectItem (item, "type");
            if (item)
                return (item->valuestring);
            break;
        }
        item = item->next;
    }
    return ("");                //  No such device or property
}


//  --------------------------------------------------------------------------
//  Creates a named 0MQ socket within a named device, and configures the
//  socket as specified in the configuration data.  Returns NULL if the device
//  or socket do not exist.
//
void *
zfl_config_socket (zfl_config_t *self, char *device, char *name, int type)
{
    void
        *socket;                //  0MQ socket
    cJSON
        *item;

    assert (self);
    assert (device);
    assert (strneq (device, "context"));

    //  Find named device
    item = self->json->child;
    while (item) {
        if (streq (item->string, device))
            break;
        item = item->next;
    }
    if (!item)
        return (NULL);          //  No such device

    socket = zmq_socket (self->context, type);
    if (!socket)
        return (NULL);          //  Can't create socket

    if (self->verbose)
        printf ("I: Configuring '%s' socket in '%s' device...\n", name, device);

    item = cJSON_GetObjectItem (item, name);
    if (item)
        s_parse_socket_item (self, socket, item, "");
    else
    if (self->verbose)
        printf ("W: No configuration found for '%s'\n", name);

    return (socket);
}


//  --------------------------------------------------------------------------
//  Returns the 0MQ context associated with this config
//
void *
zfl_config_context (zfl_config_t *self)
{
    assert (self);
    return (self->context);
}


//  --------------------------------------------------------------------------
//  Returns the verbose property
//
Bool
zfl_config_verbose (zfl_config_t *self)
{
    assert (self);
    return (self->verbose);
}


//  --------------------------------------------------------------------------
//  Selftest

int
zfl_config_test (void)
{
    zfl_config_t
        *config;
    zfl_blob_t
        *blob;
    char
        *config_name = "zfl_config_test.json";
    FILE
        *file;
    void
        *frontend,
        *backend;
    char
        *device;                //  Name of first device

    printf (" * zfl_config: ");
    //  Load config data into blob
    file = fopen (config_name, "rb");
    blob = zfl_blob_new ();
    assert (blob);
    assert (zfl_blob_load (blob, file));
    fclose (file);

    //  Create a new config from the blob data
    config = zfl_config_new (zfl_blob_data (blob));
    assert (config);

    //  Test unknown device
    frontend = zfl_config_socket (config, "nosuch", "frontend", ZMQ_SUB);
    assert (frontend == NULL);

    //  Find real device
    device = zfl_config_device (config, 0);
    assert (*device);           //  Must not be empty

    //  Configure two sockets
    frontend = zfl_config_socket (config, device, "frontend", ZMQ_SUB);
    assert (frontend);
    backend = zfl_config_socket (config, device, "backend", ZMQ_PUB);
    assert (backend);

    zfl_blob_destroy (&blob);
    zfl_config_destroy (&config);
    assert (config == NULL);

    printf ("OK\n");
    return 0;
}
