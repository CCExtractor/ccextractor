/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/socket.h>

#include <libexplain/buffer/accept4_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifndef HAVE_ACCEPT4
# ifndef SOCK_NONBLOCK
#  define SOCK_NONBLOCK 0
# endif
# ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC 0
# endif
#endif


static const explain_parse_bits_table_t table[] =
{
    { "SOCK_NONBLOCK", SOCK_NONBLOCK },
    { "SOCK_CLOEXEC", SOCK_CLOEXEC },
};


void
explain_buffer_accept4_flags(explain_string_buffer_t *sb, int flags)
{
    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


int
explain_accept4_flags_parse_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
