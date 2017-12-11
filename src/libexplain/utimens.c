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
#include <libexplain/ac/sys/types.h>

#include <libexplain/buffer/errno/utimens.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/utimens.h>


const char *
explain_utimens(const char *pathname, const struct timespec *data)
{
    return explain_errno_utimens(errno, pathname, data);
}


const char *
explain_errno_utimens(int errnum, const char *pathname, const struct timespec
    *data)
{
    explain_message_errno_utimens(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, pathname, data);
    return explain_common_message_buffer;
}


void
explain_message_utimens(char *message, int message_size, const char *pathname,
    const struct timespec *data)
{
    explain_message_errno_utimens(message, message_size, errno, pathname, data);
}


void
explain_message_errno_utimens(char *message, int message_size, int errnum, const
    char *pathname, const struct timespec *data)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_utimens(&sb, errnum, pathname, data);
}


/* vim: set ts=8 sw=4 et : */
