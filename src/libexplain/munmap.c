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
#include <libexplain/ac/sys/mman.h>

#include <libexplain/buffer/errno/munmap.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/munmap.h>


const char *
explain_munmap(void *data, size_t data_size)
{
    return explain_errno_munmap(errno, data, data_size);
}


const char *
explain_errno_munmap(int errnum, void *data, size_t data_size)
{
    explain_message_errno_munmap(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, data, data_size);
    return explain_common_message_buffer;
}


void
explain_message_munmap(char *message, int message_size, void *data, size_t
    data_size)
{
    explain_message_errno_munmap(message, message_size, errno, data, data_size);
}


void
explain_message_errno_munmap(char *message, int message_size, int errnum, void
    *data, size_t data_size)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_munmap(&sb, errnum, data, data_size);
}


/* vim: set ts=8 sw=4 et : */
