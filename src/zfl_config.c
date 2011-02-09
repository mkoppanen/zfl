/*  =========================================================================
    zfl_config.c

    Loads a configuration file formatted in JSON or in ZPL format as defined
    by rfc.zeromq.org/spec:4/zpl. Provides methods to navigate this data and
    access property values. See zfl_config.c for examples of use.

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

#include "../include/zfl_prelude.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_config.h"
#include "../include/zfl_config_json.h"
#include "../include/zfl_config_zpl.h"

//  Structure of our class

struct _zfl_config_t {
    char
        *name;                  //  Property name if any
    struct _zfl_config_t
        *child,                 //  First child if any
        *next,                  //  Next sibling if any
        *parent;                //  Parent if any
    zfl_blob_t
        *blob;                  //  Value blob, if any
};


//  --------------------------------------------------------------------------
//  Constructor
//
//  Optionally attach new config to parent config, as first or next child.
//
zfl_config_t *
zfl_config_new (char *name, zfl_config_t *parent)
{
    zfl_config_t
        *self;

    self = zmalloc (sizeof (zfl_config_t));
    zfl_config_set_name (self, name);
    if (parent) {
        if (parent->child) {
            //  Attach as last child of parent
            zfl_config_t *last = parent->child;
            while (last->next)
                last = last->next;
            last->next = self;
        }
        else
            //  Attach as first child of parent
            parent->child = self;
    }
    self->parent = parent;
    return self;
}


//  --------------------------------------------------------------------------
//  Destructor
//
void
zfl_config_destroy (zfl_config_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_config_t *self = *self_p;

        //  Recurse like it's a normal binary config
        if (self->child)
            zfl_config_destroy (&self->child);
        if (self->next)
            zfl_config_destroy (&self->next);

        zfl_blob_destroy (&self->blob);
        zfree (self->name);
        zfree (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Construct a config object and load from a specified file.
//
//  If the file looks like JSON it's parsed as such, else it's parsed as ZPL
//  (rfc.zeromq.org/spec:4/zpl). If the filename is "-", reads from STDIN.
//  Returns NULL if the file does not exist or cannot be read or has an
//  invalid syntax.
//

zfl_config_t *
zfl_config_load (char *filename)
{
    //  Open file as specified and make sure we can read it
    FILE *file;
    if (streq (filename, "-"))
        file = stdin;           //  "-" means read from stdin
    else {
        file = fopen (filename, "r");
        if (!file)
            return NULL;            //  File missing or not readable
    }
    //  Load file data into a memory blob
    zfl_blob_t *blob = zfl_blob_new (NULL, 0);
    assert (blob);
    assert (zfl_blob_load (blob, file));
    fclose (file);

    //  Autodetect whether it's JSON or ZPL text
    char *data = (char *) zfl_blob_data (blob);
    while (isspace (*data))
        data++;

    zfl_config_t *self;
    if (*data == '{')
        self = zfl_config_json (zfl_blob_data (blob));
    else
        self = zfl_config_zpl (zfl_blob_data (blob));

    zfl_blob_destroy (&blob);
    return self;
}


//  --------------------------------------------------------------------------
//  Return config child
//
zfl_config_t *
zfl_config_child (zfl_config_t *self)
{
    assert (self);
    return self->child;
}


//  --------------------------------------------------------------------------
//  Return config next
//
zfl_config_t *
zfl_config_next (zfl_config_t *self)
{
    assert (self);
    return self->next;
}


//  --------------------------------------------------------------------------
//  Finds the latest node at the specified depth, where 0 is the root.  If no
//  such node exists, returns NULL.
//
zfl_config_t *
zfl_config_at_depth (zfl_config_t *self, int level)
{
    while (level > 0) {
        if (self->child) {
            self = self->child;
            while (self->next)
                self = self->next;
            level--;
        }
        else
            return NULL;
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Finds a node specified by path, consisting of name/name/...  If the node
//  exists, returns node, else returns NULL.
//
zfl_config_t *
zfl_config_locate (zfl_config_t *self, char *path)
{
    //  Calculate significant length of name
    char *slash = strchr (path, '/');
    int length = strlen (path);
    if (slash)
        length = slash - path;

    //  Find matching name starting at first child of root
    zfl_config_t *child = self->child;
    while (child) {
        if (strlen (child->name) == length
        &&  memcmp (child->name, path, length) == 0) {
            if (slash)          //  Look deeper
                return zfl_config_locate (child, slash + 1);
            else
                return child;
        }
        child = child->next;
    }
    return NULL;
}


//  --------------------------------------------------------------------------
//  Finds a node specified by path, consisting of name/name/...  If the node
//  exists, returns its value as a string, else returns default_value.
//
char *
zfl_config_resolve (zfl_config_t *self, char *path, char *default_value)
{
    zfl_config_t *config = zfl_config_locate (self, path);
    if (config)
        return zfl_config_string (config);
    else
        return default_value;
}


//  --------------------------------------------------------------------------
//  Return config name as string.
//
char *
zfl_config_name (zfl_config_t *self)
{
    assert (self);
    return self->name;
}


//  --------------------------------------------------------------------------
//  Set config name
//  The name may be NULL.
//
int
zfl_config_set_name (zfl_config_t *self, char *name)
{
    assert (self);
    zfree (self->name);
    self->name = zstrdup (name);
    return 0;
}


//  --------------------------------------------------------------------------
//  Returns config value as blob.
//
zfl_blob_t *
zfl_config_value (zfl_config_t *self)
{
    assert (self);
    return self->blob;
}


//  --------------------------------------------------------------------------
//  Set config value from specified blob.  Note that the data is copied.
//
int
zfl_config_set_value (zfl_config_t *self, zfl_blob_t *blob)
{
    assert (self);
    zfl_blob_destroy (&self->blob);
    if (blob)
        self->blob = zfl_blob_new (zfl_blob_data (blob), zfl_blob_size (blob));
    return 0;
}


//  --------------------------------------------------------------------------
//  Returns config string value
//
char *
zfl_config_string (zfl_config_t *self)
{
    assert (self);
    if (self->blob)
        return zfl_blob_data (self->blob);
    else
        return "";
}


//  --------------------------------------------------------------------------
//  Set config value from a string.
//  The string may be NULL, which wipes the config value.
//
int
zfl_config_set_string (zfl_config_t *self, char *string)
{
    assert (self);
    zfl_blob_t *blob = zfl_blob_new (NULL, 0);
    zfl_blob_set_dptr (blob, string, string? strlen (string) + 1: 0);
    zfl_config_set_value (self, blob);
    zfl_blob_destroy (&blob);
    return 0;
}


//  --------------------------------------------------------------------------
//  Set config value from a string formatted using printf syntax.
//
int
zfl_config_set_printf (zfl_config_t *self, char *format, ...)
{
    char value [255 + 1];
    va_list args;

    assert (self);
    va_start (args, format);
    vsnprintf (value, 255, format, args);
    va_end (args);

    zfl_blob_t *blob = zfl_blob_new (NULL, 0);
    zfl_blob_set_dptr (blob, value, strlen (value) + 1);
    zfl_config_set_value (self, blob);
    zfl_blob_destroy (&blob);
    return 0;
}


//  --------------------------------------------------------------------------
//  Walks the config as a hierarchy.  Executes handler on self, then does all
//  children in a list.  That algorithm assumes there is exactly one root
//  node with no siblings.
//
static int
s_config_execute (zfl_config_t *self, zfl_config_fct handler, void *context, int level)
{
    assert (self);
    int rc = handler (self, context, level);

    //  Process all children in one go, as a list
    zfl_config_t *child = self->child;
    while (child && !rc) {
        rc = s_config_execute (child, handler, context, level + 1);
        child = child->next;
    }
    return rc;
}

int
zfl_config_execute (zfl_config_t *self, zfl_config_fct handler, void *context)
{
    //  Execute top level config at level zero
    assert (self);
    return s_config_execute (self, handler, context, 0);
}


//  --------------------------------------------------------------------------
//  Dump config to stdout in ZPL format
//
static int
s_config_dump (zfl_config_t *self, void *context, int level)
{
    assert (self);
    if (level > 0) {
        if (self->blob)
            printf ("%*s%s = %s\n", (level - 1) * 4, "",
                self->name? self->name: "(Unnamed)",
                (char *) zfl_blob_data (self->blob));
        else
            printf ("%*s%s\n", (level - 1) * 4, "",
                self->name? self->name: "(Unnamed)");
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Prints config to stdout, for debugging purposes
//
int
zfl_config_dump (zfl_config_t *self)
{
    //  Execute top level config at level zero
    assert (self);
    int rc = zfl_config_execute (self, s_config_dump, NULL);
    return rc;
}


//  --------------------------------------------------------------------------
//  Selftest
//
int
zfl_config_test (Bool verbose)
{
    //  We create a config of this structure:
    //
    //  root
    //      type = zqueue
    //      frontend
    //          option
    //              hwm = 1000
    //              swap = 25000000     #  25MB
    //              subscribe = #2
    //          bind = tcp://eth0:5555
    //      backend
    //          bind = tcp://eth0:5556
    //
    zfl_config_t
        *root,
        *type,
        *frontend,
        *option,
        *hwm,
        *swap,
        *subscribe,
        *bind,
        *backend;

    printf (" * zfl_config: ");

    //  Left is first child, next is next sibling
    root     = zfl_config_new ("root", NULL);
    type     = zfl_config_new ("type", root);
    zfl_config_set_string (type, "zqueue");
    frontend = zfl_config_new ("frontend", root);
    option   = zfl_config_new ("option", frontend);
    hwm      = zfl_config_new ("hwm", option);
    zfl_config_set_string (hwm, "1000");
    swap     = zfl_config_new ("swap", option);
    zfl_config_set_string (swap, "25000000");
    subscribe = zfl_config_new ("subscribe", option);
    zfl_config_set_printf (subscribe, "#%d", 2);
    bind     = zfl_config_new ("bind", frontend);
    zfl_config_set_string (bind, "tcp://eth0:5555");
    backend  = zfl_config_new ("backend", root);
    bind     = zfl_config_new ("bind", backend);
    zfl_config_set_string (bind, "tcp://eth0:5556");
    if (verbose) {
        puts ("");
        zfl_config_dump (root);
    }
    zfl_config_destroy (&root);
    assert (root == NULL);

    //  Test loading from a JSON file
    zfl_config_t *config;
    config = zfl_config_load ("zfl_config_test.json");
    assert (config);

    //  Test loading from a ZPL file
    config = zfl_config_load ("zfl_config_test.txt");
    assert (config);

    printf ("OK\n");
    return 0;
}
