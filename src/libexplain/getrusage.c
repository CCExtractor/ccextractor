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
#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/errno/getrusage.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/getrusage.h>


const char *
explain_getrusage(int who, struct rusage *usage)
{
    return explain_errno_getrusage(errno, who, usage);
}


const char *
explain_errno_getrusage(int errnum, int who, struct rusage *usage)
{
    explain_message_errno_getrusage(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, who, usage);
    return explain_common_message_buffer;
}


void
explain_message_getrusage(char *message, int message_size, int who, struct
    rusage *usage)
{
    explain_message_errno_getrusage(message, message_size, errno, who, usage);
}


void
explain_message_errno_getrusage(char *message, int message_size, int errnum, int
    who, struct rusage *usage)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_getrusage(&sb, errnum, who, usage);
}


/* vim: set ts=8 sw=4 et : */
