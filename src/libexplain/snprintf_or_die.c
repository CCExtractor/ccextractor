/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2012, 2013 Peter Miller
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

#include <libexplain/common_message_buffer.h>
#include <libexplain/output.h>
#include <libexplain/snprintf.h>
#include <libexplain/buffer/errno/snprintf.h>


int
explain_snprintf_or_die(char *data, size_t data_size, const char *format, ...)
{
    int             hold_errno;
    va_list         ap;
    va_list         ap2;
    int             result;

    va_start(ap, format);
    va_copy(ap2, ap); /* See comment in libexplain/fprintf_or_die.c */
    hold_errno = errno;
    errno = EINVAL;
    result = vsnprintf(data, data_size, format, ap);
    if (result < 0)
    {
        explain_string_buffer_t sb;

        hold_errno = errno;
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        /* can't re-use "ap" here, this is why we prepped "ap2" earlier */
        explain_buffer_errno_snprintf
        (
            &sb,
            hold_errno,
            data,
            data_size,
            format,
            ap2
        );
        explain_output_error("%s", explain_common_message_buffer);
        explain_output_exit_failure();
    }
    va_end(ap2); /* yes, both of them */
    va_end(ap);
    errno = hold_errno;
    return result;
}


int
explain_snprintf_on_error(char *data, size_t data_size, const char *format, ...)
{
    int             hold_errno;
    int             result;
    va_list         ap;
    va_list         ap2;

    va_start(ap, format);
    va_copy(ap2, ap); /* see comments in explain_fprintf_or_die */

    hold_errno = errno;
    errno = EINVAL;
    result = vsnprintf(data, data_size, format, ap);
    if (result < 0)
    {
        explain_string_buffer_t sb;

        hold_errno = errno;
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        explain_buffer_errno_snprintf
        (
            &sb,
            hold_errno,
            data,
            data_size,
            format,
            ap2
        );
        explain_output_error("%s", explain_common_message_buffer);
    }
    va_end(ap2); /* yes, both of them */
    va_end(ap);
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
