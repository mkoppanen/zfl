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
zfl_rpc_test (Bool verbose)
{
    zfl_rpc_t
        *rpc;

    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    if ((major * 1000 + minor * 100 + patch) < 2100) {
        printf ("E: need at least 0MQ version 2.1.0\n");
        exit (EXIT_FAILURE);
    }
    void *context = zmq_init (1);
    assert (context);

    rpc = zfl_rpc_new (context);
    assert (rpc);
    zfl_rpc_connect (rpc, "master", "tcp://127.0.0.1:5001");
    zfl_rpc_connect (rpc, "slave", "tcp://127.0.0.1:5002");

    //  Don't actually send any data since the server won't be there

    zfl_rpc_destroy (&rpc);
    assert (rpc == NULL);

    zmq_term (context);
    return 0;
}


int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_rpc_test);
}