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

#include "testutil.h"

//  --------------------------------------------------------------------------
//  Selftest

int
zfl_config_test (Bool verbose)
{
    //  We create a config of this structure:
    //
    //  root
    //      type = zqueue
    //      frontend
    //          option
    //              hwm = 1000
    //              swap = 25000000     #  25MB
    //              subscribe = #2
    //          bind = tcp://eth0:5555
    //      backend
    //          bind = tcp://eth0:5556
    //
    zfl_config_t
        *root,
        *type,
        *frontend,
        *option,
        *hwm,
        *swap,
        *subscribe,
        *bind,
        *backend;

    //  Left is first child, next is next sibling
    root     = zfl_config_new ("root", NULL);
    type     = zfl_config_new ("type", root);
    zfl_config_set_string (type, "zqueue");
    frontend = zfl_config_new ("frontend", root);
    option   = zfl_config_new ("option", frontend);
    hwm      = zfl_config_new ("hwm", option);
    zfl_config_set_string (hwm, "1000");
    swap     = zfl_config_new ("swap", option);
    zfl_config_set_string (swap, "25000000");
    subscribe = zfl_config_new ("subscribe", option);
    zfl_config_set_printf (subscribe, "#%d", 2);
    bind     = zfl_config_new ("bind", frontend);
    zfl_config_set_string (bind, "tcp://eth0:5555");
    backend  = zfl_config_new ("backend", root);
    bind     = zfl_config_new ("bind", backend);
    zfl_config_set_string (bind, "tcp://eth0:5556");
    if (verbose) {
        puts ("");
        zfl_config_save (root, "-");
    }
    zfl_config_destroy (&root);
    assert (root == NULL);

    //  Test loading from a JSON file
    zfl_config_t *config;
    config = zfl_config_load ("zfl_config_test.json");
    assert (config);
    zfl_config_destroy (&config);

    //  Test loading from a ZPL file
    config = zfl_config_load ("zfl_config_test.txt");
    assert (config);
    zfl_config_destroy (&config);

    return 0;
}

int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_config_test);
}