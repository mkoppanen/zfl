/*  =========================================================================
    zfl_tree_json.c

    Loads a JSON file into a zfl_tree structure.  Does not provide detailed
    error reporting.  To verify your JSON files use http://www.jsonlint.com.
    This version uses the cJSON library.

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

#include <zmq.h>
#include "../include/zfl_prelude.h"
#include "../include/zfl_blob.h"
#include "../include/zfl_tree.h"

//  Import the cJSON library
#include "import/cJSON/cJSON.h"
#include "import/cJSON/cJSON.c"


//  Load one JSON element, recursively
//
static void
s_load_element (zfl_tree_t *parent, cJSON *element)
{
    assert (parent);
    assert (element);

    if (element->type == cJSON_Object) {
        zfl_tree_t *tree = zfl_tree_new (element->string, parent);
        element = element->child;
        while (element) {
            s_load_element (tree, element);
            element = element->next;
        }
    }
    else
    if (element->type == cJSON_Array) {
        cJSON *value = element->child;
        assert (value);
        while (value) {
            zfl_tree_t *tree = zfl_tree_new (element->string, parent);
            if (value->type == cJSON_String)
                zfl_tree_set_string (tree, value->valuestring);
            else
                zfl_tree_set_printf (tree, "%d", value->valueint);
            value = value->next;
        }
    }
    else {
        zfl_tree_t *tree = zfl_tree_new (element->string, parent);
        if (element->type == cJSON_String)
            zfl_tree_set_string (tree, element->valuestring);
        else
            zfl_tree_set_printf (tree, "%d", element->valueint);
    }
}


//  --------------------------------------------------------------------------
//  Load JSON data into zfl_tree_t structure.  Here is an example JSON string
//  and corresponding tree structure:
//
//  {
//      "context": {
//          "iothreads": 1,
//          "verbose": false
//      },
//      "main" : {
//          "type": "queue",
//          "frontend": {
//              "option": {
//                  "hwm": 1000,
//                  "swap": 25000000
//              },
//              "bind": [ "inproc://addr1", "ipc://addr3" ]
//          },
//          "backend": {
//              "bind": "inproc://addr3"
//          }
//      }
//  }
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
//
zfl_tree_t *
zfl_tree_json (char *json_string)
{
    //  Prepare new zfl_tree_t structure
    zfl_tree_t *self = zfl_tree_new ("root", NULL);

    //  Parse JSON data
    cJSON *json = cJSON_Parse (json_string);
    if (!json)
        json = cJSON_Parse ("{ }");

    //  Load JSON data into tree where top item is unnamed
    cJSON *child = json->child;
    while (child) {
        s_load_element (self, child);
        child = child->next;
    }
    //  Delete JSON tree
    cJSON_Delete (json);
    return self;
}


//  --------------------------------------------------------------------------
//  Load JSON data from specified text file.  Returns NULL if the file does
//  not exist or can't be read by this process.
//
zfl_tree_t *
zfl_tree_json_file (char *filename)
{
    FILE *file = fopen (filename, "r");
    if (file) {
        zfl_blob_t *blob = zfl_blob_new (NULL, 0);
        assert (blob);
        assert (zfl_blob_load (blob, file));
        fclose (file);
        zfl_tree_t *tree = zfl_tree_json (zfl_blob_data (blob));
        zfl_blob_destroy (&blob);
        return tree;
    }
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Selftest
//
int
zfl_tree_json_test (Bool verbose)
{
    printf (" * zfl_tree_json: ");

    zfl_tree_t *tree = zfl_tree_json_file ("zfl_config_test.json");
    if (verbose) {
        puts ("");
        zfl_tree_dump (tree);
    }
    zfl_tree_destroy (&tree);

    printf ("OK\n");
    return 0;
}
