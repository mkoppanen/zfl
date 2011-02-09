/*  =========================================================================
    zfl_base.c - base class for ZFL

    Provides template for new classes, and canonical style guidelines for all
    ZFL source code.

    -------------------------------------------------------------------------
    Copyright (c) 1991-2011 iMatix Corporation <www.imatix.com>
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

#include "../include/zfl_prelude.h"
#include "../include/zfl_base.h"

//  Structure of our class

struct _zfl_base_t {
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

zfl_base_t *
zfl_base_new (void)
{
    zfl_base_t
        *self;

    self = zmalloc (sizeof (zfl_base_t));
    s_private_function_example();
    return self;
}

//  --------------------------------------------------------------------------
//  Destructor

void
zfl_base_destroy (zfl_base_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_base_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Filler property
//  Does nothing special but if it did, we'd explain that here

int
zfl_base_filler (zfl_base_t *self)
{
    assert (self);
    return self->filler;
}

void
zfl_base_filler_set (zfl_base_t *self, int newvalue)
{
    assert (self);
    self->filler = newvalue;
}


//  --------------------------------------------------------------------------
//  Selftest

int
zfl_base_test (Bool verbose)
{
    zfl_base_t
        *base;

    printf (" * zfl_base: ");
    base = zfl_base_new ();
    assert (base);

    zfl_base_filler_set (base, 123);
    assert (zfl_base_filler (base) == 123);

    zfl_base_destroy (&base);
    assert (base == NULL);

    printf ("OK\n");
    return 0;
}
