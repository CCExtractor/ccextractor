/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/short.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_short(explain_string_buffer_t *sb, short value)
{
    explain_string_buffer_printf(sb, "%hd", value);
}


#define QUITE_A_FEW 30


void
explain_buffer_short_star(explain_string_buffer_t *sb, const short *data,
    size_t data_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, data_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j >= QUITE_A_FEW)
        {
            explain_string_buffer_puts(sb, "...");
            break;
        }
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_short(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_ushort(explain_string_buffer_t *sb, unsigned short value)
{
    explain_string_buffer_printf(sb, "%hu", value);
}


void
explain_buffer_ushort_star(explain_string_buffer_t *sb,
    const unsigned short *data, size_t data_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, data_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j >= QUITE_A_FEW)
        {
            explain_string_buffer_puts(sb, "...");
            break;
        }
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_ushort(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
