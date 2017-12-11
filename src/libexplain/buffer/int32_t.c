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

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_int32_t(explain_string_buffer_t *sb, int32_t data)
{
    explain_string_buffer_printf(sb, "%ld", (long)data);
}


void
explain_buffer_int32_array(explain_string_buffer_t *sb, const int32_t *data,
    size_t data_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_int32_t(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_int32_array_all_zero(const int32_t *data, size_t data_size)
{
    while (data_size > 0)
    {
        if (*data)
            return 0;
        ++data;
        --data_size;
    }
    return 1;
}


void
explain_buffer_uint32_t(explain_string_buffer_t *sb, uint32_t data)
{
    explain_string_buffer_printf(sb, "%lu", (unsigned long)data);
}


void
explain_buffer_uint32_array(explain_string_buffer_t *sb, const uint32_t *data,
    size_t data_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_uint32_t(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_uint32_array_all_zero(const uint32_t *data, size_t data_size)
{
    while (data_size > 0)
    {
        if (*data)
            return 0;
        ++data;
        --data_size;
    }
    return 1;
}


/* vim: set ts=8 sw=4 et : */
