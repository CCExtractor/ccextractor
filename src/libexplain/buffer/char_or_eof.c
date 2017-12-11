/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/char.h>
#include <libexplain/buffer/char_or_eof.h>
#include <libexplain/buffer/int.h>


void
explain_buffer_char_or_eof(explain_string_buffer_t *sb, int value)
{
    unsigned char   c;

    if (value == EOF)
    {
        explain_string_buffer_puts(sb, "EOF");
        return;
    }
    c = value;
    if (isprint(c) || isspace(c))
    {
        int             high_bits;

        explain_buffer_char(sb, c);
        high_bits = value - c;
        if (high_bits != 0)
            explain_string_buffer_printf(sb, " | 0x%04X", high_bits);
    }
    else
        explain_buffer_int(sb, value);
}


/* vim: set ts=8 sw=4 et : */
