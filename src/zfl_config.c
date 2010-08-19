/*  =========================================================================
    zfl_config.c

    Creates a 0MQ context and sockets, configuring them as specified by a
    JSON configuration file.  The config file format follows a standard that
    is documented in zfl_config[7].  This is a representative example:

    {
        "verbose": true,
        "type": "forwarder",
        "iothreads": 2,
        "frontend": {
            "option": {
                "hwm": 1000,
                "subscribe": [ "coffee", "tea", "juice" ],
                "subscribe": "milk"
            },
            "connect": [ "tcp://eth0:5556", "inproc://mydevice" ],
        },
        "backend": {
            "bind": "tcp://localhost:5556"
        }
    }

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

#include "../include/zfl_prelude.h"
#include "../include/zfl_config.h"
#include <zmq.h>

//  Import the cJSON library
#include "import/cJSON/cJSON.h"
#include "import/cJSON/cJSON.c"

//  Structure of our class

struct _zfl_config_t {
    cJSON
        *json;              //  JSON tree
    void
        *context;           //  0MQ context for our process
    char
        *type;              //  Type property, if specified
    Bool
        verbose;            //  Show configuration in progress?
};

//  Handlers for socket configuration; each returns zero on success
//  or -1 on failure.
//
static int s_do_bind (void *socket, char *strvalue, int intvalue) {
    return (zmq_bind (socket, strvalue));
}
static int s_do_connect (void *socket, char *strvalue, int intvalue) {
    return (zmq_connect (socket, strvalue));
}
static int s_do_option_hwm (void *socket, char *strvalue, int intvalue) {
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_HWM, &optvalue, sizeof (optvalue)));
}
static int s_do_option_swap (void *socket, char *strvalue, int intvalue) {
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_SWAP, &optvalue, sizeof (optvalue)));
}
static int s_do_option_affinity (void *socket, char *strvalue, int intvalue) {
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_AFFINITY, &optvalue, sizeof (optvalue)));
}
static int s_do_option_identity (void *socket, char *strvalue, int intvalue) {
    return (zmq_setsockopt (socket, ZMQ_IDENTITY, strvalue, strlen (strvalue)));
}
static int s_do_option_subscribe (void *socket, char *strvalue, int intvalue) {
    return (zmq_setsockopt (socket, ZMQ_SUBSCRIBE, strvalue, strlen (strvalue)));
}
static int s_do_option_rate (void *socket, char *strvalue, int intvalue) {
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_RATE, &optvalue, sizeof (optvalue)));
}
static int s_do_option_recovery_ivl (void *socket, char *strvalue, int intvalue) {
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_RECOVERY_IVL, &optvalue, sizeof (optvalue)));
}
static int s_do_option_mcast_loop (void *socket, char *strvalue, int intvalue) {
    int64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_MCAST_LOOP, &optvalue, sizeof (optvalue)));
}
static int s_do_option_sndbuf (void *socket, char *strvalue, int intvalue) {
    uint64_t optvalue = intvalue;
    return (zmq_setsockopt (socket, ZMQ_SNDBUF, &optvalue, sizeof (optvalue)));
}
static int s_do_option_rcvbuf (void *socket, char *strvalue, int intvalue) {
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
            if (strcmp (s_helpers [index].path, path) == 0) {
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

zfl_config_t *
zfl_config_new (FILE *file)
{
    zfl_config_t
        *self = NULL;           //  New config object
    char
        *data;                  //  Data loaded from file
    long
        size;                   //  Size of file data
    cJSON
        *json;                  //  Loaded JSON root

    //  Examine file to see how large it is, then load
    fseek (file, 0, SEEK_END);
    size = ftell (file);
    data = malloc (size + 1);
    fseek (file, 0, SEEK_SET);
    assert (fread (data, 1, size, file) == size);

    //  Parse JSON data
    json = cJSON_Parse (data);
    if (json) {
        cJSON *item;
        int iothreads = 1;

        assert (self = malloc (sizeof (zfl_config_t)));
        self->json = json;
        self->verbose = 0;

        //  Parse device properties
        item = json->child;
        while (item) {
            if (strcmp (item->string, "iothreads") == 0) {
                iothreads = item->valueint;
                if (iothreads < 1) {
                    printf ("W: iothreads can't be %d\n", iothreads);
                    iothreads = 1;
                }
                if (self->verbose)
                    printf ("I: - will use %d I/O thread%s\n",
                        iothreads, iothreads > 1? "s":"");
            }
            else
            if (strcmp (item->string, "verbose") == 0) {
                self->verbose = item->valueint;
                if (self->verbose)
                    printf ("I: Configuration in progress\n");
            }
            else
            if (strcmp (item->string, "type") == 0) {
                if (self->type)
                    free (self->type);
                self->type = strdup (item->valuestring);
            }
            item = item->next;
        }
        self->context = zmq_init (iothreads);
    }
    free (data);
    return (self);
}

//  --------------------------------------------------------------------------
//  Destructor

void
zfl_config_destroy (zfl_config_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        cJSON_Delete ((*self_p)->json);
        if ((*self_p)->type)
            free ((*self_p)->type);
        free (*self_p);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Creates a 0MQ socket and configures it as specified in the configuration
//  data.  Returns NULL if the socket could not be created.
//
void *
zfl_config_socket (zfl_config_t *self, char *name, int type)
{
    void
        *socket;                //  0MQ socket
    cJSON
        *item;

    socket = zmq_socket (self->context, type);
    if (!socket)
        return (NULL);

    if (self->verbose)
        printf ("I: Configuring '%s' socket...\n", name);

    item = cJSON_GetObjectItem (self->json, name);
    if (item)
        s_parse_socket_item (self, socket, item, "");
    else
    if (self->verbose)
        printf ("W: No configuration data found for '%s'\n", name);

    return (socket);
}

//  --------------------------------------------------------------------------
//  Returns the 0MQ context associated with this config
//
void *
zfl_config_context (zfl_config_t *self)
{
    return (self->context);
}

//  --------------------------------------------------------------------------
//  Selftest

int
zfl_config_test (void)
{
    zfl_config_t
        *config;
    char
        *config_name = "zfl_config_test.json";
    FILE
        *config_file;
    void
        *frontend,
        *backend;

    config_file = fopen (config_name, "rb");
    config = zfl_config_new (config_file);
    fclose (config_file);
    assert (config);

    frontend = zfl_config_socket (config, "frontend", ZMQ_SUB);
    assert (frontend);
    backend = zfl_config_socket (config, "backend", ZMQ_PUB);
    assert (backend);

    zfl_config_destroy (&config);
    assert (config == NULL);
    return 0;
}

