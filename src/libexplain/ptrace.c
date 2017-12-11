/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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
#include <libexplain/ac/sys/ptrace.h>

#include <libexplain/buffer/errno/ptrace.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/ptrace.h>


const char *
explain_ptrace(int request, pid_t pid, void *addr, void *data)
{
    return explain_errno_ptrace(errno, request, pid, addr, data);
}


const char *
explain_errno_ptrace(int errnum, int request, pid_t pid, void *addr, void *data)
{
    explain_message_errno_ptrace(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, request, pid, addr, data);
    return explain_common_message_buffer;
}


void
explain_message_ptrace(char *message, int message_size, int request, pid_t pid,
    void *addr, void *data)
{
    explain_message_errno_ptrace(message, message_size, errno, request, pid,
        addr, data);
}


void
explain_message_errno_ptrace(char *message, int message_size, int errnum, int
    request, pid_t pid, void *addr, void *data)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_ptrace(&sb, errnum, request, pid, addr, data);
}


/* vim: set ts=8 sw=4 et : */
