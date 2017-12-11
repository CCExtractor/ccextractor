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
#include <libexplain/ac/time.h>

#include <libexplain/buffer/errno/nanosleep.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/nanosleep.h>


const char *
explain_nanosleep(const struct timespec *req, struct timespec *rem)
{
    return explain_errno_nanosleep(errno, req, rem);
}


const char *
explain_errno_nanosleep(int errnum, const struct timespec *req, struct timespec
    *rem)
{
    explain_message_errno_nanosleep(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, req, rem);
    return explain_common_message_buffer;
}


void
explain_message_nanosleep(char *message, int message_size, const struct timespec
    *req, struct timespec *rem)
{
    explain_message_errno_nanosleep(message, message_size, errno, req, rem);
}


void
explain_message_errno_nanosleep(char *message, int message_size, int errnum,
    const struct timespec *req, struct timespec *rem)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_nanosleep(&sb, errnum, req, rem);
}


/* vim: set ts=8 sw=4 et : */
