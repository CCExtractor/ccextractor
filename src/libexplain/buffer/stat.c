/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/dev_t.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/stat.h>
#include <libexplain/buffer/time_t.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/is_efault.h>


void
explain_buffer_stat(explain_string_buffer_t *sb,
    const struct stat *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ st_dev = ");
    explain_buffer_dev_t(sb, data->st_dev);
    explain_string_buffer_puts(sb, ", st_ino = ");
    explain_buffer_long(sb, data->st_ino);
    explain_string_buffer_puts(sb, ", st_mode = ");
    explain_buffer_permission_mode(sb, data->st_mode);
    explain_string_buffer_puts(sb, ", st_nlink = ");
    explain_buffer_long(sb, data->st_nlink);
    explain_string_buffer_puts(sb, ", st_ui = ");
    explain_buffer_uid(sb, data->st_uid);
    explain_string_buffer_puts(sb, ", st_gid = ");
    explain_buffer_gid(sb, data->st_gid);
    explain_string_buffer_puts(sb, ", st_rdev = ");
    explain_buffer_dev_t(sb, data->st_rdev);
    explain_string_buffer_puts(sb, ", st_size = ");
    explain_buffer_off_t(sb, data->st_size);
    explain_string_buffer_puts(sb, ", st_blksize = ");
    explain_buffer_long(sb, data->st_blksize);
    explain_string_buffer_puts(sb, ", st_block = ");
    explain_buffer_off_t(sb, data->st_blocks);
    explain_string_buffer_puts(sb, ", st_ctime = ");
    explain_buffer_time_t(sb, data->st_ctime);
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
