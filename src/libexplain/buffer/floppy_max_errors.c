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

#include <libexplain/buffer/floppy_max_errors.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_FD_H

void
explain_buffer_floppy_max_errors(explain_string_buffer_t *sb,
    const struct floppy_max_errors *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ abort = ");
    explain_buffer_uint(sb, data->abort);
    explain_string_buffer_puts(sb, ", read_track = ");
    explain_buffer_uint(sb, data->read_track);
    explain_string_buffer_puts(sb, ", reset = ");
    explain_buffer_uint(sb, data->reset);
    explain_string_buffer_puts(sb, ", recal = ");
    explain_buffer_uint(sb, data->recal);
    explain_string_buffer_puts(sb, ", reporting = ");
    explain_buffer_uint(sb, data->reporting);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_max_errors(explain_string_buffer_t *sb,
    const struct floppy_max_errors *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
