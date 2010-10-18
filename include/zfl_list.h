/*  =========================================================================
    zfl_msg.h - ZFL singly-linked list class

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

#ifndef __ZFL_LIST_H_INCLUDED__
#define __ZFL_LIST_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_list zfl_list_t;

//  Create a new linked list.
zfl_list_t *
    zfl_list_new ();

//  Destroy the list and deallocate memory pointed to by pointers kept in list.
void
    zfl_list_destroy (zfl_list_t **self_p);

//  Return the first item.
//  Note that this function does not removes the value from the list.
//  The list must contain at list one value.
void *
    zfl_list_front (zfl_list_t *self);

//  Add the pointer at the end of the list.
void
    zfl_list_append (zfl_list_t *self, void *value);

//  Remove the item from the list.
void
    zfl_list_remove (zfl_list_t *self, void *value);

//  Return the number of items in the list.
size_t
    zfl_list_size (zfl_list_t *self);

//  Selftest.
void
    zfl_list_test (int verbose);

#ifdef __cplusplus
}
#endif

#endif
