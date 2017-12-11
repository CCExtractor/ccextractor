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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/char.h>


void
explain_buffer_char(explain_string_buffer_t *sb, int c)
{
    const char      *p;

    c = (unsigned char)c;
    if (c == '\'' || c == '\\')
    {
        explain_string_buffer_putc(sb, '\'');
        explain_string_buffer_putc(sb, '\\');
        explain_string_buffer_putc(sb, c);
        explain_string_buffer_putc(sb, '\'');
        return;
    }
    if (isprint(c))
    {
        explain_string_buffer_putc(sb, '\'');
        explain_string_buffer_putc(sb, c);
        explain_string_buffer_putc(sb, '\'');
        return;
    }
    p = strchr("\aa\bb\ff\nn\rr\tt\vv", c);
    if (p)
    {
        explain_string_buffer_putc(sb, '\'');
        explain_string_buffer_putc(sb, '\\');
        explain_string_buffer_putc(sb, p[1]);
        explain_string_buffer_putc(sb, '\'');
        return;
    }
    explain_string_buffer_printf(sb, "0x%02X", c);
}


/* vim: set ts=8 sw=4 et : */
