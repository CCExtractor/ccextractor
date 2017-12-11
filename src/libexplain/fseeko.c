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
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/sys/types.h>

#include <libexplain/buffer/errno/fseeko.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fseeko.h>


const char *
explain_fseeko(FILE *fp, off_t offset, int whence)
{
    return explain_errno_fseeko(errno, fp, offset, whence);
}


const char *
explain_errno_fseeko(int errnum, FILE *fp, off_t offset, int whence)
{
    explain_message_errno_fseeko(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, fp, offset, whence);
    return explain_common_message_buffer;
}


void
explain_message_fseeko(char *message, int message_size, FILE *fp, off_t offset,
    int whence)
{
    explain_message_errno_fseeko(message, message_size, errno, fp, offset,
        whence);
}


void
explain_message_errno_fseeko(char *message, int message_size, int errnum, FILE
    *fp, off_t offset, int whence)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_fseeko(&sb, errnum, fp, offset, whence);
}


/* vim: set ts=8 sw=4 et : */
