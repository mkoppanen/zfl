/*  =========================================================================
    zfl_tests.c - run selftests

    Runs all selftests.

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
#include "../include/zfl_base.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_config.h"
#include "../include/zfl_log.h"

int main (int argc, char *argv [])
{
    printf ("Running ZFL self tests...\n");
    zfl_base_test ();
    zfl_blob_test ();
    zfl_config_test ();
    zfl_log_test ();

    printf ("Tests passed OK\n");
    return 0;
}
