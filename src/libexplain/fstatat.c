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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/errno/fstatat.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fstatat.h>


const char *
explain_fstatat(int fildes, const char *pathname, struct stat *data, int flags)
{
    return explain_errno_fstatat(errno, fildes, pathname, data, flags);
}


const char *
explain_errno_fstatat(int errnum, int fildes, const char *pathname, struct stat
    *data, int flags)
{
    explain_message_errno_fstatat(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, fildes, pathname, data,
        flags);
    return explain_common_message_buffer;
}


void
explain_message_fstatat(char *message, int message_size, int fildes, const char
    *pathname, struct stat *data, int flags)
{
    explain_message_errno_fstatat(message, message_size, errno, fildes,
        pathname, data, flags);
}


void
explain_message_errno_fstatat(char *message, int message_size, int errnum, int
    fildes, const char *pathname, struct stat *data, int flags)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_fstatat(&sb, errnum, fildes, pathname, data, flags);
}


/* vim: set ts=8 sw=4 et : */
