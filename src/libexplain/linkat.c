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
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/errno/linkat.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/linkat.h>


const char *
explain_linkat(int old_fildes, const char *old_path, int new_fildes, const char
    *new_path, int flags)
{
    return explain_errno_linkat(errno, old_fildes, old_path, new_fildes,
        new_path, flags);
}


const char *
explain_errno_linkat(int errnum, int old_fildes, const char *old_path, int
    new_fildes, const char *new_path, int flags)
{
    explain_message_errno_linkat(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, old_fildes, old_path,
        new_fildes, new_path, flags);
    return explain_common_message_buffer;
}


void
explain_message_linkat(char *message, int message_size, int old_fildes, const
    char *old_path, int new_fildes, const char *new_path, int flags)
{
    explain_message_errno_linkat(message, message_size, errno, old_fildes,
        old_path, new_fildes, new_path, flags);
}


void
explain_message_errno_linkat(char *message, int message_size, int errnum, int
    old_fildes, const char *old_path, int new_fildes, const char *new_path, int
    flags)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_linkat(&sb, errnum, old_fildes, old_path, new_fildes,
        new_path, flags);
}


/* vim: set ts=8 sw=4 et : */
