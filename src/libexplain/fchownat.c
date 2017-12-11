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

#include <libexplain/buffer/errno/fchownat.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fchownat.h>


const char *
explain_fchownat(int dirfd, const char *pathname, int owner, int group, int
    flags)
{
    return explain_errno_fchownat(errno, dirfd, pathname, owner, group, flags);
}


const char *
explain_errno_fchownat(int errnum, int dirfd, const char *pathname, int owner,
    int group, int flags)
{
    explain_message_errno_fchownat(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, dirfd, pathname, owner,
        group, flags);
    return explain_common_message_buffer;
}


void
explain_message_fchownat(char *message, int message_size, int dirfd, const char
    *pathname, int owner, int group, int flags)
{
    explain_message_errno_fchownat(message, message_size, errno, dirfd,
        pathname, owner, group, flags);
}


void
explain_message_errno_fchownat(char *message, int message_size, int errnum, int
    dirfd, const char *pathname, int owner, int group, int flags)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_fchownat(&sb, errnum, dirfd, pathname, owner, group,
        flags);
}


/* vim: set ts=8 sw=4 et : */
