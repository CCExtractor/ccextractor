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

#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/errno/gethostbyname.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/gethostbyname.h>


const char *
explain_gethostbyname(const char *name)
{
    return explain_errno_gethostbyname(h_errno, name);
}


const char *
explain_errno_gethostbyname(int errnum, const char *name)
{
    explain_message_errno_gethostbyname(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, name);
    return explain_common_message_buffer;
}


void
explain_message_gethostbyname(char *message, int message_size, const char *name)
{
    explain_message_errno_gethostbyname(message, message_size, h_errno, name);
}


void
explain_message_errno_gethostbyname(char *message, int message_size, int errnum,
    const char *name)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_gethostbyname(&sb, errnum, name);
}


/* vim: set ts=8 sw=4 et : */
