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

#include <libexplain/buffer/floppy_write_errors.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_FD_H

void
explain_buffer_floppy_write_errors(explain_string_buffer_t *sb,
    const struct floppy_write_errors *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ write_errors = ");
    explain_buffer_uint(sb, data->write_errors);
    if (data->first_error_sector)
    {
        explain_string_buffer_puts(sb, ", first_error_sector = ");
        explain_buffer_ulong(sb, data->first_error_sector);
    }
    if (data->first_error_generation)
    {
        explain_string_buffer_puts(sb, ", first_error_generation = ");
        explain_buffer_int(sb, data->first_error_generation);
    }
    if (data->last_error_sector)
    {
        explain_string_buffer_puts(sb, ", last_error_sector = ");
        explain_buffer_ulong(sb, data->last_error_sector);
    }
    if (data->last_error_generation)
    {
        explain_string_buffer_puts(sb, ", last_error_generation = ");
        explain_buffer_int(sb, data->last_error_generation);
    }
    if (data->badness)
    {
        explain_string_buffer_puts(sb, ", badness = ");
        explain_buffer_uint(sb, data->badness);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_write_errors(explain_string_buffer_t *sb,
    const struct floppy_write_errors *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
