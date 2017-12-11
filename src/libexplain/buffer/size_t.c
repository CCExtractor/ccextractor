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

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/is_efault.h>


void
explain_buffer_size_t(explain_string_buffer_t *sb, size_t value)
{
#if SIZEOF_SIZE_T > SIZEOF_LONG
    explain_string_buffer_printf(sb, "%llu", (unsigned long long)value);
#else
    explain_string_buffer_printf(sb, "%lu", (unsigned long)value);
#endif
}


void
explain_buffer_size_t_star(explain_string_buffer_t *sb, const size_t *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_size_t(sb, *data);
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
