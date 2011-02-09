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

#include "../include/zfl_prelude.h"
#include "../include/zfl_base.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_config.h"
#include "../include/zfl_config_json.h"
#include "../include/zfl_config_zpl.h"
#include "../include/zfl_device.h"
#include "../include/zfl_hash.h"
#include "../include/zfl_list.h"
#include "../include/zfl_msg.h"

int main (int argc, char *argv [])
{
    //  Enable malloc tracing if the platform supports it
    MALLOC_TRACE;

    Bool verbose;
    if (argc == 2 && streq (argv [1], "-v"))
        verbose = TRUE;
    else
        verbose = FALSE;

    printf ("Running ZFL self tests...\n");

    zfl_base_test (verbose);
    zfl_blob_test (verbose);
    zfl_device_test (verbose);
    zfl_hash_test (verbose);
    zfl_list_test (verbose);
    zfl_config_test (verbose);
    zfl_config_json_test (verbose);
    zfl_config_zpl_test (verbose);
    zfl_msg_test (verbose);

    printf ("Tests passed OK\n");
    return 0;
}
