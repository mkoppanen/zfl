/*  =========================================================================
    zfl_tests.c - run selftests

    Runs all selftests.

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

#include <zmq.h>
#include "testutil.h"

int
zfl_msg_test (Bool verbose)
{
    zfl_msg_t
        *zmsg;
    int rc;

    //  Prepare our context and sockets
    void *context = zmq_init (1);

    void *output = zmq_socket (context, ZMQ_XREQ);
   rc = zmq_bind (output, "tcp://*:5055");
    assert (rc == 0);
    void *input = zmq_socket (context, ZMQ_XREP);
   rc = zmq_connect (input, "tcp://localhost:5055");
    assert (rc == 0);

    //  Test send and receive of single-part message
    zmsg = zfl_msg_new ();
    assert (zmsg);
    zfl_msg_body_set (zmsg, "Hello");
    assert (strcmp (zfl_msg_body (zmsg), "Hello") == 0);
    zfl_msg_send (&zmsg, output);
    assert (zmsg == NULL);

    zmsg = zfl_msg_recv (input);
    assert (zfl_msg_parts (zmsg) == 2);
    if (verbose)
        zfl_msg_dump (zmsg);
    assert (strcmp (zfl_msg_body (zmsg), "Hello") == 0);

    zfl_msg_destroy (&zmsg);
    assert (zmsg == NULL);

    //  Test send and receive of multi-part message
    zmsg = zfl_msg_new ();
    zfl_msg_body_set (zmsg, "Hello");
    zfl_msg_wrap     (zmsg, "address1", "");
    zfl_msg_wrap     (zmsg, "address2", NULL);
    assert (zfl_msg_parts (zmsg) == 4);
    zfl_msg_send (&zmsg, output);

    zmsg = zfl_msg_recv (input);
    if (verbose)
        zfl_msg_dump (zmsg);
    assert (zfl_msg_parts (zmsg) == 5);
    assert (strlen (zfl_msg_address (zmsg)) == 33);
    free (zfl_msg_unwrap (zmsg));
    assert (strcmp (zfl_msg_address (zmsg), "address2") == 0);
    zfl_msg_body_fmt (zmsg, "%c%s", 'W', "orld");
    zfl_msg_send (&zmsg, output);

    zmsg = zfl_msg_recv (input);
    free (zfl_msg_unwrap (zmsg));
    assert (zfl_msg_parts (zmsg) == 4);
    assert (strcmp (zfl_msg_body (zmsg), "World") == 0);
    char *part;
    part = zfl_msg_unwrap (zmsg);
    assert (strcmp (part, "address2") == 0);
    free (part);

    //  Pull off address 1, check that empty part was dropped
    part = zfl_msg_unwrap (zmsg);
    assert (strcmp (part, "address1") == 0);
    assert (zfl_msg_parts (zmsg) == 1);
    free (part);

    //  Check that message body was correctly modified
    part = zfl_msg_pop (zmsg);
    assert (strcmp (part, "World") == 0);
    assert (zfl_msg_parts (zmsg) == 0);
    free (part);

    zfl_msg_destroy (&zmsg);
    assert (zmsg == NULL);

    zmq_close (input);
    zmq_close (output);

    zmq_term (context);
    return 0;
}


int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_msg_test);
}