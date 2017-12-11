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

#include <libexplain/buffer/pretty_size.h>


void
explain_buffer_pretty_size(explain_string_buffer_t *sb,
    unsigned long long size)
{
    const char      unit[] = "kMGTPEZY";
    const char      *up;
    double          value;

    if (size < 1024)
    {
        explain_string_buffer_printf(sb, "%d bytes", (int)size);
        return;
    }
    value = size / 1024.;
    up = unit;
    for (;;)
    {
        if (value < 9.995)
        {
            explain_string_buffer_printf(sb, "%4.2f%cB", value, *up);
            return;
        }
        if (value < 99.95)
        {
            explain_string_buffer_printf(sb, "%4.1f%cB", value, *up);
            return;
        }
        if (value < 1023.5)
        {
            explain_string_buffer_printf(sb, "%4.0f%cB", value, *up);
            return;
        }
        value /= 1024.;
        ++up;
    }
}


/* vim: set ts=8 sw=4 et : */
