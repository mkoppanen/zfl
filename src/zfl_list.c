/*  =========================================================================
    zfl_list.h - singly-linked list

    Singly-linked list implementation. Meant to be used by message queues.

    Follows the ZFL class conventions and is further developed as the ZFL
    zfl_msg class.  See http://zfl.zeromq.org for more details.

    -------------------------------------------------------------------------
    Copyright (c) 1991-2010 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the ZeroMQ Guide: http://zguide.zeromq.org

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

#include "../include/zfl_prelude.h"
#include "../include/zfl_list.h"

//  List's node.

struct _zfl_list_node {
    struct _zfl_list_node   *next;
    void                    *value;
};

struct _zfl_list {
    struct _zfl_list_node   *head;
    struct _zfl_list_node   *tail;
    size_t                  size;
};

//  --------------------------------------------------------------------------
//  List constructor

zfl_list_t *
zfl_list_new ()
{
    zfl_list_t *self = malloc (sizeof (zfl_list_t));
    assert (self);
    self->head = self->tail = NULL;
    self->size = 0;
    return self;
}

//  --------------------------------------------------------------------------
//  List destructor

void
zfl_list_destroy (zfl_list_t **self_p)
{
    struct _zfl_list_node *node, *next;

    for (node = (*self_p)->head; node != NULL; node = next) {
        free (node->value);
        next = node->next;
        free (node);
    }
    free (*self_p);
    *self_p = NULL;
}

//  --------------------------------------------------------------------------
//  Return the value at the head of list. The list must not be empty.
//  Note that this function does not remove the value from the list.

void *
zfl_list_front (zfl_list_t *self)
{
    assert (self);
    return self->head->value;
}

//  --------------------------------------------------------------------------
//  Add value at the end of the list.

void
zfl_list_append (zfl_list_t *self, void *value)
{
    struct _zfl_list_node *node;
    node = malloc (sizeof (struct _zfl_list_node));
    assert (node);
    node->value = value;
    if (self->tail)
        self->tail->next = node;
    else
        self->head = node;
    self->tail = node;
    node->next = NULL;
    self->size++;
}

//  --------------------------------------------------------------------------
//  Remove the value value from the list. The value must be stored in the list.
//  The function does not deallocate the memory poited to by the removed value.

void
zfl_list_remove (zfl_list_t *self, void *value)
{
    struct _zfl_list_node *node, *prev = NULL;

    //  First off, we need to find the list node.
    for (node = self->head; node != NULL; node = node->next) {
        if (node->value == value)
            break;
        prev = node;
    }
    assert (node);

    if (prev)
        prev->next = node->next;
    else
        self->head = node->next;

    if (node->next == NULL)
        self->tail = NULL;

    free (node);
    self->size--;
}

//  --------------------------------------------------------------------------
//  Return the number of items in the list.

size_t
zfl_list_size (zfl_list_t *self)
{
    return self->size;
}

//  --------------------------------------------------------------------------
//  Runs self test of class

void
zfl_list_test (int verbose)
{
    printf (" * zfl_list: ");

    zfl_list_t *list = zfl_list_new ();
    assert (list);
    assert (zfl_list_size (list) == 0);
    char *s1 = strdup ("first");
    assert (s1);
    char *s2 = strdup ("second");
    assert (s2);
    char *s3 = strdup ("third");
    assert (s3);
    zfl_list_append (list, s1);
    assert (zfl_list_size (list) == 1);
    zfl_list_append (list, s2);
    assert (zfl_list_size (list) == 2);
    zfl_list_append (list, s3);
    assert (zfl_list_size (list) == 3);
    char *tmp = zfl_list_front (list);
    assert (tmp == s1);
    assert (zfl_list_size (list) == 3);
    zfl_list_remove (list, s3);
    assert (zfl_list_size (list) == 2);
    free (s3);
    tmp = zfl_list_front (list);
    assert (tmp == s1);
    assert (zfl_list_size (list) == 2);
    zfl_list_remove (list, tmp);
    assert (zfl_list_size (list) == 1);
    free (tmp);
    zfl_list_remove (list, s2);
    assert (zfl_list_size (list) == 0);
    free (s2);
    zfl_list_append (list, strdup ("a"));
    zfl_list_append (list, strdup ("b"));
    zfl_list_append (list, strdup ("c"));
    assert (zfl_list_size (list) == 3);
    zfl_list_destroy (&list);
    assert (list == NULL);
    printf ("OK\n");
}
