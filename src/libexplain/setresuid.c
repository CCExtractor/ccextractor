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

#include <libexplain/buffer/errno/setresuid.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/setresuid.h>


const char *
explain_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
    return explain_errno_setresuid(errno, ruid, euid, suid);
}


const char *
explain_errno_setresuid(int errnum, uid_t ruid, uid_t euid, uid_t suid)
{
    explain_message_errno_setresuid(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, ruid, euid, suid);
    return explain_common_message_buffer;
}


void
explain_message_setresuid(char *message, int message_size, uid_t ruid, uid_t
    euid, uid_t suid)
{
    explain_message_errno_setresuid(message, message_size, errno, ruid, euid,
        suid);
}


void
explain_message_errno_setresuid(char *message, int message_size, int errnum,
    uid_t ruid, uid_t euid, uid_t suid)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_setresuid(&sb, errnum, ruid, euid, suid);
}


/* vim: set ts=8 sw=4 et : */
