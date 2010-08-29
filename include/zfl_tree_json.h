/*  =========================================================================
    zfl_tree_json.h - ZFL tree_json class

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

#ifndef __ZFL_TREE_JSON_H_INCLUDED__
#define __ZFL_TREE_JSON_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Load tree from JSON string
zfl_tree_t *
    zfl_tree_json (char *json_string);
zfl_tree_t *
    zfl_tree_json_file (char *filename);
int
    zfl_tree_json_test (Bool verbose);

#ifdef __cplusplus
}
#endif

#endif

