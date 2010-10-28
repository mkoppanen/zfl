/*  =========================================================================
    zfl_rpc_server.h - server side RPC

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

#ifndef __ZFL_RPC_SERVER_H_INCLUDED__
#define __ZFL_RPC_SERVER_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_rpc_server zfl_rpc_server_t;

zfl_rpc_server_t *
    zfl_rpc_server_new (void *zmq_context, char *server_id, char *endpoint);
void
    zfl_rpc_server_destroy (zfl_rpc_server_t **self_p);
void
    zfl_rpc_server_bind (zfl_rpc_server_t *self, char *endpoint);
zfl_msg_t *
    zfl_rpc_server_recv (zfl_rpc_server_t *self);
void
    zfl_rpc_server_send (zfl_rpc_server_t *self, zfl_msg_t **msg_p);

#ifdef __cplusplus
}
#endif

#endif
