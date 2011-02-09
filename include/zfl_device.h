/*  =========================================================================
    zfl_device.h - ZFL device class

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

#ifndef __ZFL_DEVICE_H_INCLUDED__
#define __ZFL_DEVICE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_device_t zfl_device_t;

zfl_device_t *
    zfl_device_new (char *filename);
void
    zfl_device_destroy (zfl_device_t **self_p);
void *
    zfl_device_context (zfl_device_t *self);
Bool
    zfl_device_verbose (zfl_device_t *self);
char *
    zfl_device_locate (zfl_device_t *self, int index);
char *
    zfl_device_property (zfl_device_t *self, char *device_name, char *property);
void *
    zfl_device_socket (zfl_device_t *self, char *device, char *socket_name, int type);
int
    zfl_device_test (Bool verbose);

#ifdef __cplusplus
}
#endif

#endif
