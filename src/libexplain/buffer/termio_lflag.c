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

#include <libexplain/ac/linux/termios.h>

#include <libexplain/buffer/termio_lflag.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


void
explain_buffer_termio_lflag(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "ISIG", ISIG },
        { "ICANON", ICANON },
#ifdef XCASE
        { "XCASE", XCASE },
#endif
        { "ECHO", ECHO },
        { "ECHOE", ECHOE },
        { "ECHOK", ECHOK },
        { "ECHONL", ECHONL },
        { "NOFLSH", NOFLSH },
        { "TOSTOP", TOSTOP },
        { "ECHOCTL", ECHOCTL },
        { "ECHOPRT", ECHOPRT },
        { "ECHOKE", ECHOKE },
        { "FLUSHO", FLUSHO },
        { "PENDIN", PENDIN },
        { "IEXTEN", IEXTEN },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
