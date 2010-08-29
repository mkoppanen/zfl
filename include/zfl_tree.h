/*  =========================================================================
    zfl_tree.h - ZFL tree class

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

#ifndef __ZFL_TREE_H_INCLUDED__
#define __ZFL_TREE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_tree_t zfl_tree_t;

//  Function that executes tree
typedef int (zfl_tree_fct) (zfl_tree_t *self, void *context, int level);

zfl_tree_t *
    zfl_tree_new (char *name, zfl_tree_t *parent);
void
    zfl_tree_destroy (zfl_tree_t **self_p);
zfl_tree_t *
    zfl_tree_child (zfl_tree_t *self);
zfl_tree_t *
    zfl_tree_next (zfl_tree_t *self);
char *
    zfl_tree_lookup (zfl_tree_t *self, char *path, char *default_value);
zfl_tree_t *
    zfl_tree_at_depth (zfl_tree_t *self, int level);
char *
    zfl_tree_name (zfl_tree_t *self);
int
    zfl_tree_set_name (zfl_tree_t *self, char *name);
zfl_blob_t *
    zfl_tree_value (zfl_tree_t *self);
int
    zfl_tree_set_value (zfl_tree_t *self, zfl_blob_t *blob);
char *
    zfl_tree_string (zfl_tree_t *self);
int
    zfl_tree_set_string (zfl_tree_t *self, char *string);
int
    zfl_tree_set_printf (zfl_tree_t *self, char *format, ...);
int
    zfl_tree_execute (zfl_tree_t *self, zfl_tree_fct handler, void *context);
int
    zfl_tree_dump (zfl_tree_t *self);
int
    zfl_tree_test (Bool verbose);

#ifdef __cplusplus
}
#endif

#endif

