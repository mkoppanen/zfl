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
#include "../include/zfl_tree.h"
#include "../include/zfl_tree_json.h"
#include "../include/zfl_tree_zpl.h"
#include "../include/zfl_config.h"

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
    zfl_config_test (verbose);
    zfl_tree_test (verbose);
    zfl_tree_json_test (verbose);
    zfl_tree_zpl_test (verbose);

    printf ("Tests passed OK\n");
    return 0;
}
