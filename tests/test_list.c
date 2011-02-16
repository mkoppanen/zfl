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

int
zfl_list_test (Bool verbose)
{
    zfl_list_t *list = zfl_list_new ();
    assert (list);
    assert (zfl_list_size (list) == 0);

    //  Three values we'll use as test data
    //  List values are void *, not particularly strings
    char *cheese = "boursin";
    char *bread = "baguette";
    char *wine = "bordeaux";

    zfl_list_append (list, cheese);
    assert (zfl_list_size (list) == 1);
    zfl_list_append (list, bread);
    assert (zfl_list_size (list) == 2);
    zfl_list_append (list, wine);
    assert (zfl_list_size (list) == 3);

    assert (zfl_list_first (list) == cheese);
    assert (zfl_list_size (list) == 3);
    zfl_list_remove (list, wine);
    assert (zfl_list_size (list) == 2);

    assert (zfl_list_first (list) == cheese);
    zfl_list_remove (list, cheese);
    assert (zfl_list_size (list) == 1);
    assert (zfl_list_first (list) == bread);

    zfl_list_remove (list, bread);
    assert (zfl_list_size (list) == 0);

    zfl_list_push (list, cheese);
    assert (zfl_list_size (list) == 1);
    assert (zfl_list_first (list) == cheese);

    zfl_list_push (list, bread);
    assert (zfl_list_size (list) == 2);
    assert (zfl_list_first (list) == bread);

    zfl_list_append (list, wine);
    assert (zfl_list_size (list) == 3);
    assert (zfl_list_first (list) == bread);

    zfl_list_remove (list, bread);
    assert (zfl_list_first (list) == cheese);

    zfl_list_remove (list, cheese);
    assert (zfl_list_first (list) == wine);

    zfl_list_remove (list, wine);
    assert (zfl_list_size (list) == 0);

    zfl_list_destroy (&list);
    assert (list == NULL);
    return 0;
}


int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_list_test);
}