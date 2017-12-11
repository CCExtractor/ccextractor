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

#include <libexplain/buffer/errno/getresgid.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/getresgid.h>


const char *
explain_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    return explain_errno_getresgid(errno, rgid, egid, sgid);
}


const char *
explain_errno_getresgid(int errnum, gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    explain_message_errno_getresgid(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, rgid, egid, sgid);
    return explain_common_message_buffer;
}


void
explain_message_getresgid(char *message, int message_size, gid_t *rgid, gid_t
    *egid, gid_t *sgid)
{
    explain_message_errno_getresgid(message, message_size, errno, rgid, egid,
        sgid);
}


void
explain_message_errno_getresgid(char *message, int message_size, int errnum,
    gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_getresgid(&sb, errnum, rgid, egid, sgid);
}


/* vim: set ts=8 sw=4 et : */
