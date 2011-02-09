/*  =========================================================================
    zfl_config_zpl.c

    Loads a ZPL property set as defined at http://rfc.zeromq.org/spec:4 into
    a zfl_config_t structure.  This code would be a LOT shorter in Perl :-)

    -------------------------------------------------------------------------
    Copyright (c) 1991-2011 iMatix Corporation <www.imatix.com>
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

#include <zmq.h>
#include "../include/zfl_prelude.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_config.h"

//  Store name and value in zfl_config_t config
static int
s_have_element (zfl_config_t *root, int level, char *name, char *value, int lineno)
{
    //  Navigate to parent for this element
    zfl_config_t *parent = zfl_config_at_depth (root, level);
    if (parent) {
        zfl_config_t *config = zfl_config_new (name, parent);
        if (*value)
            zfl_config_set_string (config, value);
        return 0;
    }
    else {
        fprintf (stderr, "E: (%d) indentation error\n", lineno);
        return -1;
    }
}


//  Count and verify indentation level, -1 means a syntax error
//
static int
s_collect_level (char **start, int lineno)
{
    char *readptr = *start;
    while (*readptr == ' ')
        readptr++;
    int level = (readptr - *start) / 4;
    if (level * 4 != readptr - *start) {
        fprintf (stderr, "E: (%d) indent 4 spaces at once\n", lineno);
        level = -1;
    }
    *start = readptr;
    return level;
}

//  Collect property name
//
static char *
s_collect_name (char **start, int lineno)
{
    char *readptr = *start;
    while (isalnum (**start) || **start == '/')
        (*start)++;

    size_t length = *start - readptr;
    char *name = zmalloc (length + 1);
    memcpy (name, readptr, length);
    name [length] = 0;
    if (name [0]== '/' || name [length -1] == '/') {
        fprintf (stderr, "E: (%d) '/' not valid at name start or end\n", lineno);
        zfree (name);
    }
    return name;
}


//  Checks there's no junk after value on line, returns 0 if OK else -1.
//
static int
s_verify_eoln (char *readptr, int lineno)
{
    while (*readptr) {
        if (isspace (*readptr))
            readptr++;
        else
        if (*readptr == '#')
            break;
        else {
            fprintf (stderr, "E: (%d) invalid syntax '%s'\n",
                lineno, readptr);
            return -1;
            break;
        }
    }
    return 0;
}


//  Returns value for name, or "" - if syntax error, returns NULL.
//
static char *
s_collect_value (char **start, int lineno)
{
    char *value = NULL;
    char *readptr = *start;
    int rc = 0;

    while (isspace (*readptr))
        readptr++;

    if (*readptr == '=') {
        readptr++;
        while (isspace (*readptr))
            readptr++;

        //  If value starts with quote or apost, collect it
        if (*readptr == '"' || *readptr == '\'') {
            char *endquote = strchr (readptr + 1, *readptr);
            if (endquote) {
                size_t value_length = endquote - readptr - 1;
                value = zmalloc (value_length + 1);
                memcpy (value, readptr + 1, value_length);
                value [value_length] = 0;
                rc = s_verify_eoln (endquote + 1, lineno);
            }
            else {
                fprintf (stderr, "E: (%d) missing %c\n", lineno, *readptr);
                rc = -1;
            }
        }
        else {
            //  Collect unquoted value up to comment
            char *comment = strchr (readptr, '#');
            if (comment) {
                while (isspace (comment [-1]))
                    comment--;
                *comment = 0;
            }
            value = strdup (readptr);
        }
    }
    else {
        value = strdup ("");
        rc = s_verify_eoln (readptr, lineno);
    }
    //  If we had an error, drop value and return NULL
    if (rc) {
        zfree (value);
    }
    return value;
}


//  Process current line and attach to config
//  Returns 0 on success, -1 if there was a syntax error
//
static int
s_process_line (zfl_config_t *root, char *start, int lineno)
{
    //  Did we parse the line successfully?
    int rc = 0;

    //  Strip whitespace off end of line
    int length = strlen (start);
    while (isspace (start [length - 1]))
        start [--length] = 0;

    //  Collect indentation level and name, if any
    int level = s_collect_level (&start, lineno);
    if (level == -1)
        rc = -1;

    char *name = s_collect_name (&start, lineno);
    if (name == NULL)
        rc = -1;
    else
    //  If name is not empty, collect property value
    if (*name) {
        char *value = s_collect_value (&start, lineno);
        if (value == NULL)
            rc = -1;
        else
            rc = s_have_element (root, level, name, value, lineno);
        zfree (value);
    }
    else
        rc = s_verify_eoln (start, lineno);

    zfree (name);
    return rc;
}


//  --------------------------------------------------------------------------
//  Load ZPL data into zfl_config_t structure.  Here is an example ZPL stream
//  and corresponding config structure:
//
//  context
//      iothreads = 1
//      verbose = 1      #   Ask for a trace
//  main
//      type = zqueue       #  ZMQ_DEVICE type
//      frontend
//          option
//              hwm = 1000
//              swap = 25000000     #  25MB
//          bind = 'inproc://addr1'
//          bind = 'ipc://addr2'
//      backend
//          bind = inproc://addr3
//
//  root                    Down = child
//    |                     Across = next
//    v
//  context-->main
//    |         |
//    |         v
//    |       type=queue-->frontend-->backend
//    |                      |          |
//    |                      |          v
//    |                      |        bind=inproc://addr3
//    |                      |
//    |                      v
//    |                    option-->bind=inproc://addr1-->bind=ipc://addr2
//    |                      |
//    |                      v
//    |                     hwm=1000-->swap=25000000
//    |
//    v
//  iothreads=1-->verbose=false
//
zfl_config_t *
zfl_config_zpl (char *zpl_string)
{
    //  Prepare new zfl_config_t structure
    zfl_config_t *self = zfl_config_new ("root", NULL);

    //  Process ZPL string line by line
    char *next_line = zpl_string;
    int lineno = 0;
    Bool valid = TRUE;
    while (*next_line) {
        //  Find end of line, and trim spaces
        char *eoln = strchr (next_line, '\n');
        char *this_line = next_line;
        if (eoln) {
            //  Terminate string at end of line and move to next line
            *eoln = 0;
            next_line = eoln + 1;
        }
        else {
            //  Find end of line and set up end of loop
            eoln = strchr (next_line, 0);
            assert (eoln);
            next_line = eoln;
        }
        if (s_process_line (self, this_line, ++lineno))
            valid = FALSE;
    }
    //  Either the whole ZPL file is valid or none of it is
    if (!valid) {
        zfl_config_destroy (&self);
        self = zfl_config_new ("root", NULL);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Load ZPL data from specified text file.  Returns NULL if the file does
//  not exist or can't be read by this process.
//
zfl_config_t *
zfl_config_zpl_file (char *filename)
{
    FILE *file = fopen (filename, "r");
    if (file) {
        zfl_blob_t *blob = zfl_blob_new (NULL, 0);
        assert (blob);
        assert (zfl_blob_load (blob, file));
        fclose (file);
        zfl_config_t *config = zfl_config_zpl (zfl_blob_data (blob));
        zfl_blob_destroy (&blob);
        return config;
    }
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Selftest
//
int
zfl_config_zpl_test (Bool verbose)
{
    printf (" * zfl_config_zpl: ");

    zfl_config_t *config = zfl_config_zpl_file ("zfl_config_test.txt");
    assert (config);
    if (verbose) {
        puts ("");
        zfl_config_dump (config);
    }
    zfl_config_destroy (&config);

    printf ("OK\n");
    return 0;
}
