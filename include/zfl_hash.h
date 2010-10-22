/*  =========================================================================
    zfl_hash.h - ZFL singly-linked hash class

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

#ifndef __ZFL_HASH_H_INCLUDED__
#define __ZFL_HASH_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Callback function for zfl_hash_apply method
typedef int (zfl_hash_apply_fn) (char *key, void *value, void *argument);

//  Opaque class structure
typedef struct _zfl_hash zfl_hash_t;

zfl_hash_t *
    zfl_hash_new (void);
void
    zfl_hash_destroy (zfl_hash_t **self_p);
int
    zfl_hash_insert (zfl_hash_t *self, char *key, void *value);
void
    zfl_hash_delete (zfl_hash_t *self, char *key);
void *
    zfl_hash_lookup (zfl_hash_t *self, char *key);
size_t
    zfl_hash_size (zfl_hash_t *self);
int
    zfl_hash_apply (zfl_hash_t *self, zfl_hash_apply_fn *callback, void *argument);
void
    zfl_hash_test (int verbose);

#ifdef __cplusplus
}
#endif

#endif
