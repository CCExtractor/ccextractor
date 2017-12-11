/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/int16_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_int16_t(explain_string_buffer_t *sb, int16_t data)
{
    explain_string_buffer_printf(sb, "%d", data);
}


void
explain_buffer_int16_array(explain_string_buffer_t *sb, const int16_t *data,
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
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_int16_t(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_uint16_t(explain_string_buffer_t *sb, uint16_t data)
{
    explain_string_buffer_printf(sb, "%u", data);
}


void
explain_buffer_uint16_array(explain_string_buffer_t *sb, const uint16_t *data,
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
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_uint16_t(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_uint16_array2(explain_string_buffer_t *sb, const uint16_t *data,
    size_t dim1_size, size_t dim2_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, dim1_size * dim2_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < dim1_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_uint16_array(sb, data + j * dim2_size, dim2_size);
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
