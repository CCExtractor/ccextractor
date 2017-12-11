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

#include <libexplain/ac/linux/fd.h>

#include <libexplain/buffer/format_descr.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_FD_H

void
explain_buffer_format_descr(explain_string_buffer_t *sb,
    const struct format_descr *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ device = ");
    explain_buffer_int(sb, data->device);
    explain_string_buffer_puts(sb, ", head = ");
    explain_buffer_int(sb, data->head);
    explain_string_buffer_puts(sb, ", track = ");
    explain_buffer_int(sb, data->track);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_format_descr(explain_string_buffer_t *sb,
    const struct format_descr *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
