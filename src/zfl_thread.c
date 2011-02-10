/*  =========================================================================
    zfl_thread.c - work with operating system threads

    Provides a portable API for creating, killing, and waiting on operating
    system threads. Used instead of, e.g., pthreads, which is not portable to
    all platforms.

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
#include "../include/zfl_thread.h"

//  Structure of our class

struct _zfl_thread_t {
#if defined (__UNIX__)
pthread_t
    thread;
#elif defined (__WINDOWS__)
int
    filler;                         //  To be done
#else
#   error "Platform not supported by zfl_thread class"
#endif
};


//  --------------------------------------------------------------------------
//  Constructor

zfl_thread_t *
zfl_thread_new (void *(*thread_fn) (void *), void *args)
{
    zfl_thread_t
        *self;

    self = zmalloc (sizeof (zfl_thread_t));
#if defined (__UNIX__)
    int rc = pthread_create (&self->thread, NULL, thread_fn, args);
#else
#   error "Platform not supported by zfl_thread class"
#endif
    if (rc != 0)
        zfl_thread_destroy (&self);

    return self;
}


//  --------------------------------------------------------------------------
//  Destructor

void
zfl_thread_destroy (zfl_thread_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_thread_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//
//
int
zfl_thread_wait (zfl_thread_t *self)
{
#if defined (__UNIX__)
    int rc = pthread_join (self->thread, NULL);
#else
#   error "Platform not supported by zfl_thread class"
#endif
    return rc;
}


//  --------------------------------------------------------------------------
//

int
zfl_thread_cancel (zfl_thread_t *self)
{
    return 0;
}


//  --------------------------------------------------------------------------
//  Selftest

static void *
test_thread (void *args) {
    assert (streq (args, "HELLO"));
    return NULL;
}

int
zfl_thread_test (Bool verbose)
{
    zfl_thread_t
        *thread;

    printf (" * zfl_thread: ");
    thread = zfl_thread_new (test_thread, "HELLO");
    assert (thread);
    zfl_thread_wait (thread);

    zfl_thread_destroy (&thread);
    assert (thread == NULL);

    printf ("OK\n");
    return 0;
}
