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
#include <libexplain/ac/string.h>

#include <libexplain/buffer/errno/strcoll.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/strcoll.h>


const char *
explain_strcoll(const char *s1, const char *s2)
{
    return explain_errno_strcoll(errno, s1, s2);
}


const char *
explain_errno_strcoll(int errnum, const char *s1, const char *s2)
{
    explain_message_errno_strcoll(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, s1, s2);
    return explain_common_message_buffer;
}


void
explain_message_strcoll(char *message, int message_size, const char *s1, const
    char *s2)
{
    explain_message_errno_strcoll(message, message_size, errno, s1, s2);
}


void
explain_message_errno_strcoll(char *message, int message_size, int errnum, const
    char *s1, const char *s2)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_strcoll(&sb, errnum, s1, s2);
}


/* vim: set ts=8 sw=4 et : */
