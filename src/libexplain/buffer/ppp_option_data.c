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

#include <libexplain/ac/net/if_ppp.h>

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/ppp_option_data.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef PPPIOCSCOMPRESS

void
explain_buffer_ppp_option_data(explain_string_buffer_t *sb,
    const struct ppp_option_data *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ptr = ");
    if (explain_is_efault_pointer(data->ptr, data->length))
        explain_buffer_pointer(sb, data->ptr);
    else
        explain_buffer_char_data(sb, data->ptr, data->length);
    explain_string_buffer_puts(sb, ", length = ");
    explain_buffer_int32_t(sb, data->length);
    explain_string_buffer_puts(sb, ", transmit = ");
    explain_buffer_int(sb, data->transmit);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_ppp_option_data(explain_string_buffer_t *sb,
    const struct ppp_option_data *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
