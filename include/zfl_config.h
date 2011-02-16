/*  =========================================================================
    zfl_config.h - ZFL config class

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

#ifndef __ZFL_CONFIG_H_INCLUDED__
#define __ZFL_CONFIG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zfl_config_t zfl_config_t;

//  Function that executes config
typedef int (zfl_config_fct) (zfl_config_t *self, void *arg, int level);

zfl_config_t *
    zfl_config_new (char *name, zfl_config_t *parent);
void
    zfl_config_destroy (zfl_config_t **self_p);
zfl_config_t *
    zfl_config_load (char *filename);
int
    zfl_config_save (zfl_config_t *self, char *filename);
zfl_config_t *
    zfl_config_child (zfl_config_t *self);
zfl_config_t *
    zfl_config_next (zfl_config_t *self);
zfl_config_t *
    zfl_config_locate (zfl_config_t *self, char *path);
char *
    zfl_config_resolve (zfl_config_t *self, char *path, char *default_value);
zfl_config_t *
    zfl_config_at_depth (zfl_config_t *self, int level);
char *
    zfl_config_name (zfl_config_t *self);
int
    zfl_config_set_name (zfl_config_t *self, char *name);
zfl_blob_t *
    zfl_config_value (zfl_config_t *self);
int
    zfl_config_set_value (zfl_config_t *self, zfl_blob_t *blob);
char *
    zfl_config_string (zfl_config_t *self);
int
    zfl_config_set_string (zfl_config_t *self, char *string);
int
    zfl_config_set_printf (zfl_config_t *self, char *format, ...);
int
    zfl_config_execute (zfl_config_t *self, zfl_config_fct handler, void *arg);
int
    zfl_config_dump (zfl_config_t *self);

#ifdef __cplusplus
}
#endif

#endif

