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

#include <libexplain/string_buffer.h>


void
explain_string_buffer_path_join(explain_string_buffer_t *dst,
    const char *s)
{
    while (s[0] == '.' && s[1] == '/')
        s += 2;
    if (s[0] == '.' && s[1] == '\0')
        return;
    if (s[0] == '\0')
        return;
    if (dst->position == 1 && dst->message[0] == '.')
        dst->position = 0;
    else if (dst->position > 0 && dst->message[dst->position - 1] != '/')
        explain_string_buffer_putc(dst, '/');
    explain_string_buffer_puts(dst, s);
}


/* vim: set ts=8 sw=4 et : */
