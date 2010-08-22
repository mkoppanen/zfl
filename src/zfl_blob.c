/*  =========================================================================
    zfl_blob.c - binary long object

    Manipulates opaque binary objects including reading and writing from/to
    files.  Example use case is for loading config data from stdin or file
    for processing by zfl_config.

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
#include "../include/zfl_blob.h"

//  Structure of our class

struct _zfl_blob_t {
    size_t
        size;                   //  Blob data size
    byte
        *data;                  //  Content of our blob
};


//  --------------------------------------------------------------------------
//  Constructor
//
zfl_blob_t *
zfl_blob_new (void)
{
    zfl_blob_t
        *self;

    self = malloc (sizeof (zfl_blob_t));
    self->data = NULL;
    self->size = 0;
    return (self);
}

//  --------------------------------------------------------------------------
//  Destructor
//
void
zfl_blob_destroy (zfl_blob_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        if ((*self_p)->data)
            free ((*self_p)->data);
        free (*self_p);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Loads blob from file.  Always adds a binary zero to end of blob data so
//  that it can be parsed as a string if necessary.  Returns size of blob
//  data.  Idempotent, does not change current read position in file. If
//  file cannot be read, returns empty blob.
//
size_t
zfl_blob_load (zfl_blob_t *self, FILE *file)
{
    long
        cur_position,
        end_position;

    //  In any case, drop any previous data we had
    zfl_blob_init (self);

    //  Get current position in file so we can come back here afterwards
    cur_position = ftell (file);
    if (fseek (file, 0, SEEK_END) == 0) {
        //  Now determine actual size of blob in file
        end_position = ftell (file);
        assert (end_position >= 0);

        //  Allocate new blob data with extra byte for null
        self->size = (size_t) end_position;
        self->data = malloc (self->size + 1);

        //  Read blob data
        fseek (file, 0, SEEK_SET);
        assert (fread (self->data, 1, self->size, file) == self->size);
        self->data [self->size] = 0;

        //  Restore original file position
        fseek (file, cur_position, SEEK_SET);
    }
    return (self->size);
}


//  --------------------------------------------------------------------------
//  Initializes blob data (sets to empty)
//
int
zfl_blob_init (zfl_blob_t *self)
{
    assert (self);
    if (self->data)
        free (self->data);
    self->size = 0;
    return (0);
}


//  --------------------------------------------------------------------------
//  Returns size of blob data.
//
size_t
zfl_blob_size (zfl_blob_t *self)
{
    assert (self);
    return (self->size);
}


//  --------------------------------------------------------------------------
//  Returns pointer to blob data.
//
void *
zfl_blob_data (zfl_blob_t *self)
{
    assert (self);
    return (self->data);
}


//  --------------------------------------------------------------------------
//  Sets blob data as specified.  Always appends a null byte to the data.
//  Use like this:
//
//      zfl_blob_data_set (blob, intval, sizeof (intval));
//      zfl_blob_data_set (blob, strval, strlen (strval));
//
int
zfl_blob_data_set (zfl_blob_t *self, void *data, size_t size)
{
    assert (self);
    assert (data);
    assert (size >= 0);

    zfl_blob_init (self);
    self->size = size;
    self->data = malloc (self->size + 1);
    memcpy (self->data, data, size);
    self->data [self->size] = 0;

    return (0);
}


//  --------------------------------------------------------------------------
//  Selftest
//
int
zfl_blob_test (void)
{
    zfl_blob_t
        *blob;
    char
        *string = "This is a string";
    FILE
        *file;

    printf (" * zfl_blob: ");
    blob = zfl_blob_new ();
    assert (blob);
    assert (zfl_blob_size (blob) == 0);

    file = fopen ("zfl_blob.c", "rb");
    assert (file);
    assert (zfl_blob_load (blob, file));
    assert (zfl_blob_size (blob) > 0);

    zfl_blob_data_set (blob, string, strlen (string));
    assert (zfl_blob_size (blob) == strlen (string));
    assert (streq ((char *) (zfl_blob_data (blob)), string));

    zfl_blob_destroy (&blob);
    assert (blob == NULL);

    printf ("OK\n");
    return 0;
}
