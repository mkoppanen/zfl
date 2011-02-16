/*  =========================================================================
    testutil.h - run selftests

    Utilities for running tests

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

#ifndef __ZFL_TEST_H__
# define __ZFL_TEST_H__

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
#include "../include/zfl_rpc.h"
#include "../include/zfl_rpcd.h"
#include "../include/zfl_thread.h"

typedef int (*zfl_test_function)(Bool verbose);

int zfl_test_runner (int argc, char *argv [], zfl_test_function f);

#endif /* __ZFL_TEST_H__ */
