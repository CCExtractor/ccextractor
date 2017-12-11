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

#include <libexplain/asprintf.h>
#include <libexplain/buffer/errno/asprintf.h>
#include <libexplain/common_message_buffer.h>


const char *
explain_asprintf(char **data, const char *format, ...)
{
    int             errnum;
    va_list         ap;
    explain_string_buffer_t sb;

    errnum = errno;
    va_start(ap, format);
    explain_string_buffer_init
    (
        &sb,
        explain_common_message_buffer,
        explain_common_message_buffer_size
    );
    explain_buffer_errno_asprintf(&sb, errnum, data, format, ap);
    va_end(ap);
    return explain_common_message_buffer;
}


const char *
explain_errno_asprintf(int errnum, char **data, const char *format, ...)
{
    va_list         ap;
    explain_string_buffer_t sb;

    va_start(ap, format);
    explain_string_buffer_init
    (
        &sb,
        explain_common_message_buffer,
        explain_common_message_buffer_size
    );
    explain_buffer_errno_asprintf(&sb, errnum, data, format, ap);
    va_end(ap);
    return explain_common_message_buffer;
}


void
explain_message_asprintf(char *message, int message_size, char **data,
    const char *format, ...)
{
    va_list         ap;
    int             errnum;
    explain_string_buffer_t sb;

    errnum = errno;
    va_start(ap, format);
    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_asprintf(&sb, errnum, data, format, ap);
    va_end(ap);
}


void
explain_message_errno_asprintf(char *message, int message_size, int errnum,
    char **data, const char *format, ...)
{
    va_list         ap;
    explain_string_buffer_t sb;

    va_start(ap, format);
    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_asprintf(&sb, errnum, data, format, ap);
    va_end(ap);
}


/* vim: set ts=8 sw=4 et : */
