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

#include <libexplain/ac/sys/eventfd.h>

#include <libexplain/buffer/eventfd_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifndef EFD_NONBLOCK
#define EFD_NONBLOCK 0
#endif
#ifndef EFD_CLOEXEC
#define EFD_CLOEXEC 0
#endif


static const explain_parse_bits_table_t table[] =
{
    { "EFD_NONBLOCK", EFD_NONBLOCK },
    { "EFD_CLOEXEC", EFD_CLOEXEC },
};


void
explain_buffer_eventfd_flags(explain_string_buffer_t *sb, int flags)
{
    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


int
explain_parse_eventfd_flags_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
