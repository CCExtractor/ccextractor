/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/mbstate_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


void
explain_buffer_mbstate_t(explain_string_buffer_t *sb, const mbstate_t *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
#ifdef HAVE__G_CONFIG_H
    if (explain_option_dialect_specific())
    {
        explain_string_buffer_puts(sb, "__count = ");
        explain_buffer_int(sb, data->__count);
        if (data->__count > 0)
        {
            size_t          n;

            n = data->__count;
            if (n > sizeof(data->__value.__wchb))
                n = sizeof(data->__value.__wchb);
            explain_string_buffer_puts(sb, ", __wchb = ");
            explain_buffer_char_data(sb, data->__value.__wchb, n);
        }
    }
    else
#endif
    {
        explain_string_buffer_puts(sb, "<undefined>");
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
