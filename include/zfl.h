/*  =========================================================================
    zfl.h - ZFL wrapper

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

#ifndef __ZFL_H_INCLUDED__
#define __ZFL_H_INCLUDED__

//  Always include ZeroMQ header file
//
#include <zmq.h>

//  Set up environment for the application
//
#include <zfl_prelude.h>

//  Classes listed in alphabetical order except for dependencies
//
#include <zfl_base.h>
#include <zfl_blob.h>
#include <zfl_msg.h>
#include <zfl_tree.h>
#include <zfl_tree_json.h>
#include <zfl_tree_zpl.h>
#include <zfl_config.h>

#endif
