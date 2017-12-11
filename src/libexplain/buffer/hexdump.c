/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/hexdump.h>


void
explain_buffer_hexdump(explain_string_buffer_t *sb, const void *data,
    size_t data_size)
{
    const unsigned char *cp;

    cp = data;
    while (data_size > 0)
    {
        explain_string_buffer_printf(sb, " %02X", *cp++);
        --data_size;
    }
}


void
explain_buffer_hexdump_array(explain_string_buffer_t *sb, const void *data,
    size_t data_size)
{
    const unsigned char *cp;
    size_t          j;

    explain_string_buffer_puts(sb, "{ ");
    cp = data;
    for (j = 0; j < data_size; ++j)
    {
        unsigned char   c;

        if (j)
            explain_string_buffer_puts(sb, ", ");
        c = cp[j];
        if (isprint(c) || isspace(c))
            explain_string_buffer_putc_quoted(sb, c);
        else
            explain_string_buffer_printf(sb, "0x%02X", *cp++);
    }
    explain_string_buffer_puts(sb, " }");
}


static int
mostly_text(const char *data, size_t data_size)
{
    size_t          bin;
    size_t          j;

    bin = 0;
    for (j = 0; j < data_size; ++j)
    {
        unsigned char c = data[j];
        if (!c)
            return 0;
        if (!isprint(c) && !isspace(c))
            ++bin;
    }
    return (bin * 4 < data_size);
}


void
explain_buffer_mostly_text(explain_string_buffer_t *sb, const void *data,
    size_t data_size)
{
    if (mostly_text(data, data_size))
        explain_string_buffer_puts_quoted_n(sb, data, data_size);
    else
        explain_buffer_hexdump_array(sb, data, data_size);
}


/* vim: set ts=8 sw=4 et : */
