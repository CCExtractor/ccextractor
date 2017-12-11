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

#include <libexplain/buffer/fpos_t.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/mbstate_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


void
explain_buffer_fpos_t(explain_string_buffer_t *sb, const fpos_t *data,
    int complete)
{
    if (!complete || explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
#ifdef HAVE__G_CONFIG_H
    if (explain_option_dialect_specific())
    {
        explain_string_buffer_puts(sb, "__pos = ");
        explain_buffer_int64_t(sb, data->__pos);
        explain_string_buffer_puts(sb, ", __state = ");
        explain_buffer_mbstate_t(sb, &data->__state);
    }
    else
#endif
    {
        explain_string_buffer_puts(sb, "<undefined>");
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
