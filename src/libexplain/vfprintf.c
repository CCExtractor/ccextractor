/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/errno/vfprintf.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/vfprintf.h>


const char *
explain_vfprintf(FILE *fp, const char *format, va_list ap)
{
    return explain_errno_vfprintf(errno, fp, format, ap);
}


const char *
explain_errno_vfprintf(int errnum, FILE *fp, const char *format, va_list ap)
{
    explain_message_errno_vfprintf(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, fp, format, ap);
    return explain_common_message_buffer;
}


void
explain_message_vfprintf(char *message, int message_size, FILE *fp, const char
    *format, va_list ap)
{
    explain_message_errno_vfprintf(message, message_size, errno, fp, format,
        ap);
}


void
explain_message_errno_vfprintf(char *message, int message_size, int errnum, FILE
    *fp, const char *format, va_list ap)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_vfprintf(&sb, errnum, fp, format, ap);
}


/* vim: set ts=8 sw=4 et : */
