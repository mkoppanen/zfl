/*  =========================================================================
    zfl_msg.h - ZFL msg class

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

#ifndef __ZFL_MSG_H_INCLUDED__
#define __ZFL_MSG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_msg_t zfl_msg_t;

//  Constructor and destructor
zfl_msg_t *
    zfl_msg_new (void);
void
    zfl_msg_destroy (zfl_msg_t **self_p);

//  Receive and send message, wrapping new/destroy
zfl_msg_t *
    zfl_msg_recv (void *socket);
void
    zfl_msg_send (zfl_msg_t **self, void *socket);

//  Report size of message
size_t
    zfl_msg_parts (zfl_msg_t *self);

//  Read and set message body part as C string
char
    *zfl_msg_body (zfl_msg_t *self);
void
    zfl_msg_body_set (zfl_msg_t *self, char *body);
void
    zfl_msg_body_fmt (zfl_msg_t *self, char *format, ...);

//  Generic push/pop message part off front
void
    zfl_msg_push (zfl_msg_t *self, char *part);
char
    *zfl_msg_pop (zfl_msg_t *self);

//  Read and set message envelopes
char
    *zfl_msg_address (zfl_msg_t *self);
void
    zfl_msg_wrap (zfl_msg_t *self, char *address, char *delim);
char
    *zfl_msg_unwrap (zfl_msg_t *self);

//  Dump message to stderr, for debugging and tracing
void
    zfl_msg_dump (zfl_msg_t *self);

//  Selftest for the class
int
    zfl_msg_test (int verbose);

#ifdef __cplusplus
}
#endif

#endif
