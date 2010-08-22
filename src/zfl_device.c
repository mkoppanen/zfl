/*  =========================================================================
    zfl_device.c - device class

    Provides functionality to configure and start devices.

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

#include "../include/zfl_prelude.h"
#include "../include/zfl_config.h"
#include "../include/zfl_device.h"

//  Structure of our class

struct _zfl_device_t {
    int
        filler;                         //  An example property
};

//  Private functions

static int
s_private_function_example (void)
{
    return 0;
}

//  --------------------------------------------------------------------------
//  Constructor

zfl_device_t *
zfl_device_new (void)
{
    zfl_device_t
        *self;

    self = malloc (sizeof (zfl_device_t));
    memset (self, 0, sizeof (zfl_device_t));
    return (self);
}

//  --------------------------------------------------------------------------
//  Destructor

void
zfl_device_destroy (zfl_device_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        free (*self_p);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Filler property
//  Does nothing special but if it did, we'd explain that here

int
zfl_device_filler (zfl_device_t *self)
{
    assert (self);
    return (self->filler);
}

void
zfl_device_filler_set (zfl_device_t *self, int newvalue)
{
    assert (self);
    self->filler = newvalue;
}


//  --------------------------------------------------------------------------
//  Selftest

int
zfl_device_test (void)
{
    zfl_device_t
        *device;

    printf (" * zfl_device: ");
    device = zfl_device_new ();
    assert (device);

    zfl_device_filler_set (device, 123);
    assert (zfl_device_filler (device) == 123);

    zfl_device_destroy (&device);
    assert (device == NULL);

    printf ("OK\n");
    return 0;
}
