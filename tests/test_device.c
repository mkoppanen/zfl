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
zfl_device_test (Bool verbose)
{
    //  Create a new device from the ZPL test file
    zfl_device_t *device = zfl_device_new ("zfl_device_test.txt");
    assert (device);

    //  Look for device that does not exist
    void *socket = zfl_device_socket (device, "nosuch", "socket", ZMQ_SUB);
    assert (socket == NULL);
    zmq_close (socket);

    //  Look for existing device, configure it
    char *main_device = zfl_device_locate (device, 0);
    assert (*main_device);
    assert (streq (main_device, "main"));

    char *type = zfl_device_property (device, main_device, "type");
    assert (*type);
    assert (streq (type, "zmq_queue"));

    char *endpoint = zfl_device_property (device, main_device, "frontend/endpoint");
    assert (*endpoint);
    assert (streq (endpoint, "valid-endpoint"));

    //  Configure two sockets
    void *frontend = zfl_device_socket (device, main_device, "frontend", ZMQ_SUB);
    assert (frontend);
    zmq_close (frontend);

    void *backend = zfl_device_socket (device, main_device, "backend", ZMQ_PUB);
    assert (backend);
    zmq_close (backend);

    zfl_device_destroy (&device);
    assert (device == NULL);
    return 0;
}


int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_device_test);
}