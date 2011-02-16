/*  =========================================================================
    zfl_thread.h - work with operating system threads

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

#ifndef __ZFL_THREAD_H_INCLUDED__
#define __ZFL_THREAD_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_thread_t zfl_thread_t;

zfl_thread_t *
    zfl_thread_new (void *(*thread_fn) (void *), void *args);
void
    zfl_thread_destroy (zfl_thread_t **self_p);
int
    zfl_thread_wait (zfl_thread_t *self);
int
    zfl_thread_cancel (zfl_thread_t *self);

#ifdef __cplusplus
}
#endif

#endif
