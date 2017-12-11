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

#include <libexplain/buffer/floppy_drive_struct.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/short.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_FD_H

static void
explain_buffer_floppy_drive_struct_flags(explain_string_buffer_t *sb,
    unsigned long data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "FD_NEED_TWADDLE", FD_NEED_TWADDLE },
        { "FD_VERIFY", FD_VERIFY },
        { "FD_DISK_NEWCHANGE", FD_DISK_NEWCHANGE },
        { "FD_DISK_CHANGED", FD_DISK_CHANGED },
        { "FD_DISK_WRITABLE", FD_DISK_WRITABLE },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
};


void
explain_buffer_floppy_drive_struct(explain_string_buffer_t *sb,
    const struct floppy_drive_struct *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ flags = ");
    explain_buffer_floppy_drive_struct_flags(sb, data->flags);
    if (data->spinup_date)
    {
        explain_string_buffer_puts(sb, ", spinup_date = ");
        explain_buffer_ulong(sb, data->spinup_date);
    }
    if (data->select_date)
    {
        explain_string_buffer_puts(sb, ", select_date = ");
        explain_buffer_ulong(sb, data->select_date);
    }
    if (data->first_read_date)
    {
        explain_string_buffer_puts(sb, ", first_read_date = ");
        explain_buffer_ulong(sb, data->first_read_date);
    }
    if (data->probed_format)
    {
        explain_string_buffer_puts(sb, ", probed_format = ");
        explain_buffer_short(sb, data->probed_format);
    }
    if (data->track)
    {
        explain_string_buffer_puts(sb, ", track = ");
        explain_buffer_short(sb, data->track);
    }
    if (data->maxblock)
    {
        explain_string_buffer_puts(sb, ", maxblock = ");
        explain_buffer_short(sb, data->maxblock);
    }
    if (data->maxtrack)
    {
        explain_string_buffer_puts(sb, ", maxtrack = ");
        explain_buffer_short(sb, data->maxtrack);
    }
    if (data->generation)
    {
        explain_string_buffer_puts(sb, ", generation = ");
        explain_buffer_int(sb, data->generation);
    }
    if (data->keep_data)
    {
        explain_string_buffer_puts(sb, ", keep_data = ");
        explain_buffer_int(sb, data->keep_data);
    }
    if (data->fd_ref)
    {
        explain_string_buffer_puts(sb, ", fd_ref = ");
        explain_buffer_int(sb, data->fd_ref);
    }
    if (data->fd_device)
    {
        explain_string_buffer_puts(sb, ", fd_device = ");
        explain_buffer_int(sb, data->fd_device);
    }
    if (data->last_checked)
    {
        explain_string_buffer_puts(sb, ", last_checked = ");
        explain_buffer_ulong(sb, data->last_checked);
    }
    if (data->dmabuf)
    {
        explain_string_buffer_puts(sb, ", dmabuf = ");
        explain_buffer_pointer(sb, data->dmabuf);
    }
    if (data->bufblocks)
    {
        explain_string_buffer_puts(sb, ", bufblocks = ");
        explain_buffer_int(sb, data->bufblocks);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_drive_struct(explain_string_buffer_t *sb,
    const struct floppy_drive_struct *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
