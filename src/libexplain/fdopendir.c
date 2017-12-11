/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/dirent.h>
#include <libexplain/ac/errno.h>

#include <libexplain/buffer/errno/fdopendir.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fdopendir.h>


const char *
explain_fdopendir(int fildes)
{
    return explain_errno_fdopendir(errno, fildes);
}


const char *
explain_errno_fdopendir(int errnum, int fildes)
{
    explain_message_errno_fdopendir(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, fildes);
    return explain_common_message_buffer;
}


void
explain_message_fdopendir(char *message, int message_size, int fildes)
{
    explain_message_errno_fdopendir(message, message_size, errno, fildes);
}


void
explain_message_errno_fdopendir(char *message, int message_size, int errnum,
    int fildes)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_fdopendir(&sb, errnum, fildes);
}


/* vim: set ts=8 sw=4 et : */
