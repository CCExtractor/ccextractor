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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/errno/lchmod.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/lchmod.h>


const char *
explain_lchmod(const char *pathname, mode_t mode)
{
    return explain_errno_lchmod(errno, pathname, mode);
}


const char *
explain_errno_lchmod(int errnum, const char *pathname, mode_t mode)
{
    explain_message_errno_lchmod(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, pathname, mode);
    return explain_common_message_buffer;
}


void
explain_message_lchmod(char *message, int message_size, const char *pathname,
    mode_t mode)
{
    explain_message_errno_lchmod(message, message_size, errno, pathname, mode);
}


void
explain_message_errno_lchmod(char *message, int message_size, int errnum, const
    char *pathname, mode_t mode)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_lchmod(&sb, errnum, pathname, mode);
}


/* vim: set ts=8 sw=4 et : */
