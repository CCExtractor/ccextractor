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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/errno/fchmod.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fchmod.h>


const char *
explain_fchmod(int fildes, mode_t mode)
{
    return explain_errno_fchmod(errno, fildes, mode);
}


void
explain_message_fchmod(char *message, int message_size, int fildes, mode_t mode)
{
    explain_message_errno_fchmod(message, message_size, errno, fildes, mode);
}


void
explain_message_errno_fchmod(char *message, int message_size, int errnum,
    int fildes, mode_t mode)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_fchmod(&sb, errnum, fildes, mode);
}


const char *
explain_errno_fchmod(int errnum, int fildes, mode_t mode)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init
    (
        &sb,
        explain_common_message_buffer,
        explain_common_message_buffer_size
    );
    explain_buffer_errno_fchmod(&sb, errnum, fildes, mode);
    return explain_common_message_buffer;
}


/* vim: set ts=8 sw=4 et : */
