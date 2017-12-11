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

#include <libexplain/buffer/errno/mmap.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/mmap.h>


const char *
explain_mmap(void *data, size_t data_size, int prot, int flags, int fildes,
    off_t offset)
{
    return explain_errno_mmap(errno, data, data_size, prot, flags, fildes,
        offset);
}


const char *
explain_errno_mmap(int errnum, void *data, size_t data_size, int prot, int
    flags, int fildes, off_t offset)
{
    explain_message_errno_mmap(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, data, data_size, prot,
        flags, fildes, offset);
    return explain_common_message_buffer;
}


void
explain_message_mmap(char *message, int message_size, void *data, size_t
    data_size, int prot, int flags, int fildes, off_t offset)
{
    explain_message_errno_mmap(message, message_size, errno, data, data_size,
        prot, flags, fildes, offset);
}


void
explain_message_errno_mmap(char *message, int message_size, int errnum, void
    *data, size_t data_size, int prot, int flags, int fildes, off_t offset)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_mmap(&sb, errnum, data, data_size, prot, flags, fildes,
        offset);
}


/* vim: set ts=8 sw=4 et : */
