/*  =========================================================================
    zfl_config.h - ZFL config class

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

#ifndef __ZFL_CONFIG_H_INCLUDED__
#define __ZFL_CONFIG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_config_t zfl_config_t;

zfl_config_t *
    zfl_config_new (zfl_tree_t *tree);
void
    zfl_config_destroy (zfl_config_t **self_p);
char *
    zfl_config_device (zfl_config_t *self, int index);
char *
    zfl_config_device_type (zfl_config_t *self, char *device);
void *
    zfl_config_socket (zfl_config_t *self, char *device, char *name, int type);
void *
    zfl_config_context (zfl_config_t *self);
Bool
    zfl_config_verbose (zfl_config_t *self);
int
    zfl_config_test (Bool verbose);

#ifdef __cplusplus
}
#endif

#endif
