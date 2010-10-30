/*  =========================================================================
    zfl_msg.h - multipart 0MQ message

    Multipart message class for 0MQ applications.

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

#include <zmq.h>
#include "../include/zfl_prelude.h"
#include "../include/zfl_msg.h"

//  Pretty arbitrary limit on complexity of a message
#define ZFL_MSG_MAX_PARTS  255

//  Structure of our class
//  We access these properties only via class methods

struct _zfl_msg_t {
    //  Part data follows message recv/send order
    byte  *_part_data [ZFL_MSG_MAX_PARTS];
    size_t _part_size [ZFL_MSG_MAX_PARTS];
    size_t _part_count;
};


//  --------------------------------------------------------------------------
//  Constructor

zfl_msg_t *
zfl_msg_new (void)
{
    zfl_msg_t
        *self;

    self = zmalloc (sizeof (zfl_msg_t));
    return self;
}

//  --------------------------------------------------------------------------
//  Destructor

void
zfl_msg_destroy (zfl_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zfl_msg_t *self = *self_p;

        //  Free message parts, if any
        while (self->_part_count)
            free (zfl_msg_pop (self));

        //  Free object structure
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Formats 17-byte UUID as 33-char string starting with '@'
//  Lets us print UUIDs as C strings and use them as addresses
//
static char *
s_encode_uuid (byte *data)
{
    static char
        hex_char [] = "0123456789ABCDEF";

    assert (data [0] == 0);
    char *uuidstr = zmalloc (34);
    uuidstr [0] = '@';
    int byte_nbr;
    for (byte_nbr = 0; byte_nbr < 16; byte_nbr++) {
        uuidstr [byte_nbr * 2 + 1] = hex_char [data [byte_nbr + 1] >> 4];
        uuidstr [byte_nbr * 2 + 2] = hex_char [data [byte_nbr + 1] & 15];
    }
    uuidstr [33] = 0;
    return uuidstr;
}


//  --------------------------------------------------------------------------
//  Formats 17-byte UUID as 33-char string starting with '@'
//  Lets us print UUIDs as C strings and use them as addresses
//
static byte *
s_decode_uuid (char *uuidstr)
{
    static char
        hex_to_bin [128] = {
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,    /*   0..9     */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   A..F     */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   a..f     */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };  /*            */

    assert (strlen (uuidstr) == 33);
    assert (uuidstr [0] == '@');
    byte *data = zmalloc (17);
    int byte_nbr;
    data [0] = 0;
    for (byte_nbr = 0; byte_nbr < 16; byte_nbr++)
        data [byte_nbr + 1]
            = (hex_to_bin [uuidstr [byte_nbr * 2 + 1] & 127] << 4)
            + (hex_to_bin [uuidstr [byte_nbr * 2 + 2] & 127]);

    return data;
}


//  --------------------------------------------------------------------------
//  Private helper function to store a single message part

static void
s_set_part (zfl_msg_t *self, int part_nbr, byte *data, size_t size)
{
    self->_part_size [part_nbr] = size;
    self->_part_data [part_nbr] = zmalloc (size + 1);
    memcpy (self->_part_data [part_nbr], data, size);
    //  Convert to C string if needed
    self->_part_data [part_nbr][size] = 0;
}


//  --------------------------------------------------------------------------
//  Duplicate message

zfl_msg_t *
zfl_msg_dup (zfl_msg_t *self)
{
    assert (self);
    zfl_msg_t *dup = zfl_msg_new ();
    assert (dup);

    int part_nbr;
    for (part_nbr = 0; part_nbr < self->_part_count; part_nbr++)
        s_set_part (dup, part_nbr,
            self->_part_data [part_nbr], self->_part_size [part_nbr]);
    dup->_part_count = self->_part_count;

    return dup;
}


//  --------------------------------------------------------------------------
//  Receive message from socket
//  Creates a new message and returns it
//  Blocks on recv if socket is not ready for input

zfl_msg_t *
zfl_msg_recv (void *socket)
{
    assert (socket);

    zfl_msg_t *self = zfl_msg_new ();
    while (1) {
        assert (self->_part_count < ZFL_MSG_MAX_PARTS);

        zmq_msg_t message;
        zmq_msg_init (&message);
        if (zmq_recv (socket, &message, 0)) {
            if (errno != ETERM)
                printf ("E: %s\n", zmq_strerror (errno));
            exit (1);
        }
        //  We handle 0MQ UUIDs as printable strings
        byte  *data = zmq_msg_data (&message);
        size_t size = zmq_msg_size (&message);
        if (size == 17 && data [0] == 0) {
            //  Store message part as string uuid
            char *uuidstr = s_encode_uuid (data);
            self->_part_size [self->_part_count] = strlen (uuidstr);
            self->_part_data [self->_part_count] = (byte *) uuidstr;
            self->_part_count++;
        }
        else
            //  Store this message part
            s_set_part (self, self->_part_count++, data, size);

        zmq_msg_close (&message);

        int64_t more;
        size_t more_size = sizeof (more);
        zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
        if (!more)
            break;      //  Last message part
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Send message to socket
//  Destroys message after sending

void
zfl_msg_send (zfl_msg_t **self_p, void *socket)
{
    assert (self_p);
    assert (*self_p);
    assert (socket);
    zfl_msg_t *self = *self_p;

    int part_nbr;
    for (part_nbr = 0; part_nbr < self->_part_count; part_nbr++) {
        //  Could be improved to use zero-copy since we destroy
        //  the message parts after sending anyhow...
        zmq_msg_t message;

        //  Unmangle 0MQ identities for writing to the socket
        byte  *data = self->_part_data [part_nbr];
        size_t size = self->_part_size [part_nbr];
        if (size == 33 && data [0] == '@') {
            byte *uuidbin = s_decode_uuid ((char *) data);
            zmq_msg_init_size (&message, 17);
            memcpy (zmq_msg_data (&message), uuidbin, 17);
            free (uuidbin);
        }
        else {
            zmq_msg_init_size (&message, size);
            memcpy (zmq_msg_data (&message), data, size);
        }
        int rc = zmq_send (socket, &message,
            part_nbr < self->_part_count - 1? ZMQ_SNDMORE: 0);
        assert (rc == 0);
        zmq_msg_close (&message);
    }
    zfl_msg_destroy (self_p);
}


//  --------------------------------------------------------------------------
//  Report size of message

size_t
zfl_msg_parts (zfl_msg_t *self)
{
    return self->_part_count;
}


//  --------------------------------------------------------------------------
//  Return pointer to message body, if any
//  Caller should not modify the provided data

char *
zfl_msg_body (zfl_msg_t *self)
{
    assert (self);

    if (self->_part_count)
        return (char *) self->_part_data [self->_part_count - 1];
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Set message body as copy of provided string
//  If message is empty, creates a new message body

void
zfl_msg_body_set (zfl_msg_t *self, char *body)
{
    assert (self);
    assert (body);

    if (self->_part_count) {
        assert (self->_part_data [self->_part_count - 1]);
        free (self->_part_data [self->_part_count - 1]);
    }
    else
        self->_part_count = 1;

    s_set_part (self, self->_part_count - 1, (void *) body, strlen (body));
}


//  --------------------------------------------------------------------------
//  Set message body using printf format
//  If message is empty, creates a new message body
//  Hard-coded to max. 255 characters for this simplified class

void
zfl_msg_body_fmt (zfl_msg_t *self, char *format, ...)
{
    char value [255 + 1];
    va_list args;

    assert (self);
    va_start (args, format);
    vsnprintf (value, sizeof (value), format, args);
    va_end (args);
    zfl_msg_body_set (self, value);
}


//  --------------------------------------------------------------------------
//  Push message part to front of message parts

void
zfl_msg_push (zfl_msg_t *self, char *part)
{
    assert (self);
    assert (part);
    assert (self->_part_count < ZFL_MSG_MAX_PARTS - 1);

    //  Move part stack up one element and insert new part
    memmove (&self->_part_data [1], &self->_part_data [0],
        (ZFL_MSG_MAX_PARTS - 1) * sizeof (byte *));
    memmove (&self->_part_size [1], &self->_part_size [0],
        (ZFL_MSG_MAX_PARTS - 1) * sizeof (size_t));
    s_set_part (self, 0, (void *) part, strlen (part));
    self->_part_count += 1;
}


//  --------------------------------------------------------------------------
//  Pop message part off front of message parts
//  Caller should free returned string when finished with it

char *
zfl_msg_pop (zfl_msg_t *self)
{
    assert (self);
    assert (self->_part_count);

    //  Remove first part and move part stack down one element
    char *part = (char *) self->_part_data [0];
    memmove (&self->_part_data [0], &self->_part_data [1],
        (ZFL_MSG_MAX_PARTS - 1) * sizeof (byte *));
    memmove (&self->_part_size [0], &self->_part_size [1],
        (ZFL_MSG_MAX_PARTS - 1) * sizeof (size_t));
    self->_part_count--;
    return part;
}


//  --------------------------------------------------------------------------
//  Return pointer to outer message address, if any
//  Caller should not modify the provided data

char *
zfl_msg_address (zfl_msg_t *self)
{
    assert (self);

    if (self->_part_count)
        return (char *) self->_part_data [0];
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Wraps message in new address envelope
//  If delim is not null, creates two-part envelope

void
zfl_msg_wrap (zfl_msg_t *self, char *address, char *delim)
{
    assert (self);
    assert (address);

    //  Push optional delimiter and then address
    if (delim)
        zfl_msg_push (self, delim);
    zfl_msg_push (self, address);
}


//  --------------------------------------------------------------------------
//  Unwraps outer message envelope and returns address
//  Discards empty message part after address, if any
//  Caller should free returned string when finished with it

char *
zfl_msg_unwrap (zfl_msg_t *self)
{
    assert (self);

    char *address = zfl_msg_pop (self);
    if (*zfl_msg_address (self) == 0)
        free (zfl_msg_pop (self));
    return address;
}


//  --------------------------------------------------------------------------
//  Dump message to stderr, for debugging and tracing

void
zfl_msg_dump (zfl_msg_t *self)
{
    int part_nbr;
    for (part_nbr = 0; part_nbr < self->_part_count; part_nbr++) {
        byte  *data = self->_part_data [part_nbr];
        size_t size = self->_part_size [part_nbr];

        //  Dump the message as text or binary
        int is_text = 1;
        int char_nbr;
        for (char_nbr = 0; char_nbr < size; char_nbr++)
            if (data [char_nbr] < 32 || data [char_nbr] > 127)
                is_text = 0;

        fprintf (stderr, "[%03d] ", (int) size);
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            if (is_text)
                fprintf (stderr, "%c", data [char_nbr]);
            else
                fprintf (stderr, "%02X", (byte) data [char_nbr]);
        }
        fprintf (stderr, "\n");
    }
    fflush (stderr);
}



//  --------------------------------------------------------------------------
//  Runs self test of class

int
zfl_msg_test (int verbose)
{
    zfl_msg_t
        *zmsg;
    int rc;

    printf (" * zfl_msg: ");

    //  Prepare our context and sockets
    void *context = zmq_init (1);

    void *output = zmq_socket (context, ZMQ_XREQ);
    rc = zmq_bind (output, "ipc://zfl_msg_selftest.ipc");
    assert (rc == 0);
    void *input = zmq_socket (context, ZMQ_XREP);
    rc = zmq_connect (input, "ipc://zfl_msg_selftest.ipc");
    assert (rc == 0);

    //  Test send and receive of single-part message
    zmsg = zfl_msg_new ();
    assert (zmsg);
    zfl_msg_body_set (zmsg, "Hello");
    assert (strcmp (zfl_msg_body (zmsg), "Hello") == 0);
    zfl_msg_send (&zmsg, output);
    assert (zmsg == NULL);

    zmsg = zfl_msg_recv (input);
    assert (zfl_msg_parts (zmsg) == 2);
    if (verbose)
        zfl_msg_dump (zmsg);
    assert (strcmp (zfl_msg_body (zmsg), "Hello") == 0);

    zfl_msg_destroy (&zmsg);
    assert (zmsg == NULL);

    //  Test send and receive of multi-part message
    zmsg = zfl_msg_new ();
    zfl_msg_body_set (zmsg, "Hello");
    zfl_msg_wrap     (zmsg, "address1", "");
    zfl_msg_wrap     (zmsg, "address2", NULL);
    assert (zfl_msg_parts (zmsg) == 4);
    zfl_msg_send (&zmsg, output);

    zmsg = zfl_msg_recv (input);
    if (verbose)
        zfl_msg_dump (zmsg);
    assert (zfl_msg_parts (zmsg) == 5);
    assert (strlen (zfl_msg_address (zmsg)) == 33);
    free (zfl_msg_unwrap (zmsg));
    assert (strcmp (zfl_msg_address (zmsg), "address2") == 0);
    zfl_msg_body_fmt (zmsg, "%c%s", 'W', "orld");
    zfl_msg_send (&zmsg, output);

    zmsg = zfl_msg_recv (input);
    free (zfl_msg_unwrap (zmsg));
    assert (zfl_msg_parts (zmsg) == 4);
    assert (strcmp (zfl_msg_body (zmsg), "World") == 0);
    char *part;
    part = zfl_msg_unwrap (zmsg);
    assert (strcmp (part, "address2") == 0);
    free (part);

    //  Pull off address 1, check that empty part was dropped
    part = zfl_msg_unwrap (zmsg);
    assert (strcmp (part, "address1") == 0);
    assert (zfl_msg_parts (zmsg) == 1);
    free (part);

    //  Check that message body was correctly modified
    part = zfl_msg_pop (zmsg);
    assert (strcmp (part, "World") == 0);
    assert (zfl_msg_parts (zmsg) == 0);
    free (part);

    zfl_msg_destroy (&zmsg);
    assert (zmsg == NULL);

    zmq_close (input);
    zmq_close (output);

    printf ("OK\n");
    zmq_term (context);
    return 0;
}
