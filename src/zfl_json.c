/*  =========================================================================
    zfl_json.c - JSON data parsing

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
#include "../include/zfl_json.h"

//  Structure of our class

struct _zfl_json_t {
    int
        filler;                         //  An example property
};


//  --------------------------------------------------------------------------
//  Constructor

zfl_json_t *
zfl_json_new (void)
{
    zfl_json_t
        *self;

    self = malloc (sizeof (zfl_json_t));
    memset (self, 0, sizeof (zfl_json_t));
    return (self);
}

//  --------------------------------------------------------------------------
//  Destructor

void
zfl_json_destroy (zfl_json_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        free (*self_p);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Selftest

int
zfl_json_test (void)
{
    zfl_json_t
        *self;

    self = zfl_json_new ();
    assert (self);

    zfl_json_destroy (&self);
    assert (self == NULL);
    return 0;
}
