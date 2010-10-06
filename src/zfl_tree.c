/*  =========================================================================
    zfl_tree.c - hierarchical tree

    It's a sideways binary tree that represents children and siblings.

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
#include "../include/zfl_tree.h"

//  Structure of our class

struct _zfl_tree_t {
    char
        *name;                  //  Property name if any
    struct _zfl_tree_t
        *child,                 //  First child if any
        *next,                  //  Next sibling if any
        *parent;                //  Parent if any
    zfl_blob_t
        *blob;                  //  Value blob, if any
};


//  --------------------------------------------------------------------------
//  Constructor
//
//  Optionally attach new tree to parent tree, as first or next child.
//
zfl_tree_t *
zfl_tree_new (char *name, zfl_tree_t *parent)
{
    zfl_tree_t
        *self;

    self = zmalloc (sizeof (zfl_tree_t));
    zfl_tree_set_name (self, name);
    if (parent) {
        if (parent->child) {
            //  Attach as last child of parent
            zfl_tree_t *last = parent->child;
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
zfl_tree_destroy (zfl_tree_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_tree_t *self = *self_p;

        //  Recurse like it's a normal binary tree
        if (self->child)
            zfl_tree_destroy (&self->child);
        if (self->next)
            zfl_tree_destroy (&self->next);

        zfl_blob_destroy (&self->blob);
        zfree (self->name);
        zfree (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return tree child
//
zfl_tree_t *
zfl_tree_child (zfl_tree_t *self)
{
    assert (self);
    return self->child;
}


//  --------------------------------------------------------------------------
//  Return tree next
//
zfl_tree_t *
zfl_tree_next (zfl_tree_t *self)
{
    assert (self);
    return self->next;
}


//  --------------------------------------------------------------------------
//  Finds the latest node at the specified depth, where 0 is the root.  If no
//  such node exists, returns NULL.
//
zfl_tree_t *
zfl_tree_at_depth (zfl_tree_t *self, int level)
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
zfl_tree_t *
zfl_tree_locate (zfl_tree_t *self, char *path)
{
    //  Calculate significant length of name
    char *slash = strchr (path, '/');
    int length = strlen (path);
    if (slash)
        length = slash - path;

    //  Find matching name starting at first child of root
    zfl_tree_t *child = self->child;
    while (child) {
        if (strlen (child->name) == length
        &&  memcmp (child->name, path, length) == 0) {
            if (slash)          //  Look deeper
                return zfl_tree_locate (child, slash + 1);
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
zfl_tree_resolve (zfl_tree_t *self, char *path, char *default_value)
{
    zfl_tree_t *tree = zfl_tree_locate (self, path);
    if (tree)
        return zfl_tree_string (tree);
    else
        return default_value;
}


//  --------------------------------------------------------------------------
//  Return tree name as string.
//
char *
zfl_tree_name (zfl_tree_t *self)
{
    assert (self);
    return self->name;
}


//  --------------------------------------------------------------------------
//  Set tree name
//  The name may be NULL.
//
int
zfl_tree_set_name (zfl_tree_t *self, char *name)
{
    assert (self);
    zfree (self->name);
    self->name = zstrdup (name);
    return 0;
}


//  --------------------------------------------------------------------------
//  Returns tree value as blob.
//
zfl_blob_t *
zfl_tree_value (zfl_tree_t *self)
{
    assert (self);
    return self->blob;
}


//  --------------------------------------------------------------------------
//  Set tree value from specified blob.  Note that the data is copied.
//
int
zfl_tree_set_value (zfl_tree_t *self, zfl_blob_t *blob)
{
    assert (self);
    zfl_blob_destroy (&self->blob);
    if (blob)
        self->blob = zfl_blob_new (zfl_blob_data (blob), zfl_blob_size (blob));
    return 0;
}


//  --------------------------------------------------------------------------
//  Returns tree string value
//
char *
zfl_tree_string (zfl_tree_t *self)
{
    assert (self);
    if (self->blob)
        return zfl_blob_data (self->blob);
    else
        return "";
}


//  --------------------------------------------------------------------------
//  Set tree value from a string.
//  The string may be NULL, which wipes the tree value.
//
int
zfl_tree_set_string (zfl_tree_t *self, char *string)
{
    assert (self);
    zfl_blob_t *blob = zfl_blob_new (NULL, 0);
    zfl_blob_set_dptr (blob, string, string? strlen (string) + 1: 0);
    zfl_tree_set_value (self, blob);
    zfl_blob_destroy (&blob);
    return 0;
}


//  --------------------------------------------------------------------------
//  Set tree value from a string formatted using printf syntax.
//
int
zfl_tree_set_printf (zfl_tree_t *self, char *format, ...)
{
    char value [255 + 1];
    va_list args;

    assert (self);
    va_start (args, format);
    vsnprintf (value, 255, format, args);
    va_end (args);

    zfl_blob_t *blob = zfl_blob_new (NULL, 0);
    zfl_blob_set_dptr (blob, value, strlen (value) + 1);
    zfl_tree_set_value (self, blob);
    zfl_blob_destroy (&blob);
    return 0;
}


//  --------------------------------------------------------------------------
//  Walks the tree as a hierarchy.  Executes handler on self, then does all
//  children in a list.  That algorithm assumes there is exactly one root
//  node with no siblings.
//
static int
s_tree_execute (zfl_tree_t *self, zfl_tree_fct handler, void *context, int level)
{
    assert (self);
    int rc = handler (self, context, level);

    //  Process all children in one go, as a list
    zfl_tree_t *child = self->child;
    while (child && !rc) {
        rc = s_tree_execute (child, handler, context, level + 1);
        child = child->next;
    }
    return rc;
}

int
zfl_tree_execute (zfl_tree_t *self, zfl_tree_fct handler, void *context)
{
    //  Execute top level tree at level zero
    assert (self);
    return s_tree_execute (self, handler, context, 0);
}


//  --------------------------------------------------------------------------
//  Dump tree to stdout in ZPL format
//
static int
s_tree_dump (zfl_tree_t *self, void *context, int level)
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
//  Prints tree to stdout, for debugging purposes
//
int
zfl_tree_dump (zfl_tree_t *self)
{
    //  Execute top level tree at level zero
    assert (self);
    int rc = zfl_tree_execute (self, s_tree_dump, NULL);
    return rc;
}


//  --------------------------------------------------------------------------
//  Selftest
//
//  We create a tree of this structure:
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
int
zfl_tree_test (Bool verbose)
{
    zfl_tree_t
        *root,
        *type,
        *frontend,
        *option,
        *hwm,
        *swap,
        *subscribe,
        *bind,
        *backend;

    printf (" * zfl_tree: ");

    //  Left is first child, next is next sibling
    root     = zfl_tree_new ("root", NULL);
    type     = zfl_tree_new ("type", root);
    zfl_tree_set_string (type, "zqueue");
    frontend = zfl_tree_new ("frontend", root);
    option   = zfl_tree_new ("option", frontend);
    hwm      = zfl_tree_new ("hwm", option);
    zfl_tree_set_string (hwm, "1000");
    swap     = zfl_tree_new ("swap", option);
    zfl_tree_set_string (swap, "25000000");
    subscribe = zfl_tree_new ("subscribe", option);
    zfl_tree_set_printf (subscribe, "#%d", 2);
    bind     = zfl_tree_new ("bind", frontend);
    zfl_tree_set_string (bind, "tcp://eth0:5555");
    backend  = zfl_tree_new ("backend", root);
    bind     = zfl_tree_new ("bind", backend);
    zfl_tree_set_string (bind, "tcp://eth0:5556");
    if (verbose) {
        puts ("");
        zfl_tree_dump (root);
    }
    zfl_tree_destroy (&root);
    assert (root == NULL);
    printf ("OK\n");
    return 0;
}
