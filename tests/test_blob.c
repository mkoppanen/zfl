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
zfl_blob_test (Bool verbose)
{
    zfl_blob_t
        *blob;
    char
        *string = "This is a string";
    FILE
        *file;

    blob = zfl_blob_new (NULL, 0);
    assert (blob);
    assert (zfl_blob_size (blob) == 0);

    file = fopen ("testutil.h", "r");
    assert (file);
    assert (zfl_blob_load (blob, file));
    fclose (file);

    assert (zfl_blob_size (blob) > 0);
    zfl_blob_set_data (blob, (byte *) string, strlen (string));
    assert (zfl_blob_size (blob) == strlen (string));
    assert (streq ((char *) (zfl_blob_data (blob)), string));

    zfl_blob_destroy (&blob);
    assert (blob == NULL);

    return 0;
}

int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_blob_test);
}