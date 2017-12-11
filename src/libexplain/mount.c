/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/mount.h>

#include <libexplain/buffer/errno/mount.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/mount.h>


const char *
explain_mount(const char *source, const char *target, const char
    *file_systems_type, unsigned long flags, const void *data)
{
    return explain_errno_mount(errno, source, target, file_systems_type, flags,
        data);
}


const char *
explain_errno_mount(int errnum, const char *source, const char *target, const
    char *file_systems_type, unsigned long flags, const void *data)
{
    explain_message_errno_mount(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, source, target,
        file_systems_type, flags, data);
    return explain_common_message_buffer;
}


void
explain_message_mount(char *message, int message_size, const char *source, const
    char *target, const char *file_systems_type, unsigned long flags, const void
    *data)
{
    explain_message_errno_mount(message, message_size, errno, source, target,
        file_systems_type, flags, data);
}


void
explain_message_errno_mount(char *message, int message_size, int errnum, const
    char *source, const char *target, const char *file_systems_type, unsigned
    long flags, const void *data)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_mount(&sb, errnum, source, target, file_systems_type,
        flags, data);
}


/* vim: set ts=8 sw=4 et : */
