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

#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/timerfd.h>

#include <libexplain/buffer/timerfd_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifndef TFD_NONBLOCK
#define TFD_NONBLOCK O_NONBLOCK
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#ifndef TFD_CLOEXEC
#define TFD_CLOEXEC O_CLOEXEC
#endif

static const explain_parse_bits_table_t table[] =
{
    { "TFD_NONBLOCK", TFD_NONBLOCK },
    { "TFD_CLOEXEC", TFD_CLOEXEC },
};


void
explain_buffer_timerfd_flags(explain_string_buffer_t *sb, int flags)
{
    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


int
explain_parse_timerfd_flags_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
