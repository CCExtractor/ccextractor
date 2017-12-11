/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/buffer/errno/setreuid.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/setreuid.h>


const char *
explain_setreuid(uid_t ruid, uid_t euid)
{
    return explain_errno_setreuid(errno, ruid, euid);
}


const char *
explain_errno_setreuid(int errnum, uid_t ruid, uid_t euid)
{
    explain_message_errno_setreuid(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, ruid, euid);
    return explain_common_message_buffer;
}


void
explain_message_setreuid(char *message, int message_size, uid_t ruid, uid_t
    euid)
{
    explain_message_errno_setreuid(message, message_size, errno, ruid, euid);
}


void
explain_message_errno_setreuid(char *message, int message_size, int errnum,
    uid_t ruid, uid_t euid)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_setreuid(&sb, errnum, ruid, euid);
}


/* vim: set ts=8 sw=4 et : */
