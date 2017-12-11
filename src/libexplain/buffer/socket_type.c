/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/socket.h>

#include <libexplain/parse_bits.h>
#include <libexplain/buffer/socket_type.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef SOCK_STREAM
    { "SOCK_STREAM", SOCK_STREAM },
#endif
#ifdef SOCK_DGRAM
    { "SOCK_DGRAM", SOCK_DGRAM },
#endif
#ifdef SOCK_RAW
    { "SOCK_RAW", SOCK_RAW },
#endif
#ifdef SOCK_RDM
    { "SOCK_RDM", SOCK_RDM },
#endif
#ifdef SOCK_SEQPACKET
    { "SOCK_SEQPACKET", SOCK_SEQPACKET },
#endif
#ifdef SOCK_PACKET
    { "SOCK_PACKET", SOCK_PACKET },
#endif
};


void
explain_buffer_socket_type(explain_string_buffer_t *sb, int type)
{
    const explain_parse_bits_table_t *tp;

    tp = explain_parse_bits_find_by_value(type, table, SIZEOF(table));
    if (tp)
        explain_string_buffer_puts(sb, tp->name);
    else
        explain_string_buffer_printf(sb, "%d", type);
}


int
explain_parse_socket_type_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


void
explain_buffer_socket_type_from_fildes(explain_string_buffer_t *sb,
    int fildes)
{
    int             val;
    socklen_t       valsiz;

    valsiz = sizeof(val);
    if (getsockopt(fildes, SOL_SOCKET, SO_TYPE, &val, &valsiz) >= 0)
    {
        explain_string_buffer_puts(sb, " (");
        explain_buffer_socket_type(sb, val);
        explain_string_buffer_putc(sb, ')');
    }
}


/* vim: set ts=8 sw=4 et : */
