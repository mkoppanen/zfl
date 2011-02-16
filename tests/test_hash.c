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
zfl_hash_test (Bool verbose)
{
    zfl_hash_t *hash = zfl_hash_new ();
    assert (hash);
    assert (zfl_hash_size (hash) == 0);

    //  Insert some values
    int rc;
    rc = zfl_hash_insert (hash, "DEADBEEF", (void *) 0xDEADBEEF);
    assert (rc == 0);
    rc = zfl_hash_insert (hash, "ABADCAFE", (void *) 0xABADCAFE);
    assert (rc == 0);
    rc = zfl_hash_insert (hash, "C0DEDBAD", (void *) 0xC0DEDBAD);
    assert (rc == 0);
    rc = zfl_hash_insert (hash, "DEADF00D", (void *) 0xDEADF00D);
    assert (rc == 0);
    assert (zfl_hash_size (hash) == 4);

    //  Look for existing values
    void *value;
    value = zfl_hash_lookup (hash, "DEADBEEF");
    assert (value == (void *) 0xDEADBEEF);
    value = zfl_hash_lookup (hash, "ABADCAFE");
    assert (value == (void *) 0xABADCAFE);
    value = zfl_hash_lookup (hash, "C0DEDBAD");
    assert (value == (void *) 0xC0DEDBAD);
    value = zfl_hash_lookup (hash, "DEADF00D");
    assert (value == (void *) 0xDEADF00D);

    //  Look for non-existent values
    value = zfl_hash_lookup (hash, "0xF0000000");
    assert (value == NULL);

    //  Try to insert duplicate values
    rc = zfl_hash_insert (hash, "DEADBEEF", (void *) 0xF0000000);
    assert (rc == -1);
    value = zfl_hash_lookup (hash, "DEADBEEF");
    assert (value == (void *) 0xDEADBEEF);

    //  Delete a value
    zfl_hash_delete (hash, "DEADBEEF");
    value = zfl_hash_lookup (hash, "DEADBEEF");
    assert (value == NULL);
    assert (zfl_hash_size (hash) == 3);

    //  Check that the queue is robust against random usage
    struct {
        char name [100];
        Bool exists;
    } testset [200];
    memset (testset, 0, sizeof (testset));

    int
        testmax = 200,
        testnbr,
        iteration;

    srandom ((unsigned) time (NULL));
    for (iteration = 0; iteration < 25000; iteration++) {
        testnbr = randof (testmax);
        if (testset [testnbr].exists) {
            value = zfl_hash_lookup (hash, testset [testnbr].name);
            assert (value);
            zfl_hash_delete (hash, testset [testnbr].name);
            testset [testnbr].exists = FALSE;
        }
        else {
            sprintf (testset [testnbr].name, "%x-%x", rand (), rand ());
            if (zfl_hash_insert (hash, testset [testnbr].name, "") == 0)
                testset [testnbr].exists = TRUE;
        }
    }
    //  Test 1M lookups
    for (iteration = 0; iteration < 1000000; iteration++)
        value = zfl_hash_lookup (hash, "DEADBEEFABADCAFE");

    zfl_hash_destroy (&hash);

    assert (hash == NULL);
    return 0;
}


int main (int argc, char *argv [])
{
    MALLOC_TRACE
    zfl_test_runner (argc, argv, zfl_hash_test);
}