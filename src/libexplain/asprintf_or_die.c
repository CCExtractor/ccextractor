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
#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/asprintf.h>
#include <libexplain/buffer/errno/asprintf.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/output.h>
#include <libexplain/string_buffer.h>


int
explain_asprintf_or_die(char **data, const char *format, ...)
{
    int             result;
    int             hold_errno;
    va_list         ap;
    va_list         ap2;

    hold_errno = errno;
    errno = 0;
    va_start(ap, format);

    /*
     * To run arg list twice, we need two separate ap.
     * For some discussion about this see the Linux va_copy man page.
     */
    va_copy(ap2, ap);

#ifdef HAVE_ASPRINTF
    result = vasprintf(data, format, ap);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0 || errno != 0)
    {
        explain_string_buffer_t sb;

        hold_errno = errno;
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        explain_buffer_errno_asprintf(&sb, hold_errno, data, format, ap2);
        explain_output_error_and_die("%s", explain_common_message_buffer);
        va_end(ap2);
    }
    va_end(ap);
    errno = hold_errno;
    return result;
}


int
explain_asprintf_on_error(char **data, const char *format, ...)
{
    int             result;
    int             hold_errno;
    va_list         ap;
    va_list         ap2;

    hold_errno = errno;
    errno = 0;
    va_start(ap, format);

    /*
     * To run arg list twice, we need two separate ap.
     * For some discussion about this see the Linux va_copy man page.
     */
    va_copy(ap2, ap);

#ifdef HAVE_ASPRINTF
    result = vasprintf(data, format, ap);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0 || errno != 0)
    {
        explain_string_buffer_t sb;

        hold_errno = errno;
        result = -1;
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        explain_buffer_errno_asprintf(&sb, hold_errno, data, format, ap2);
        explain_output_error("%s", explain_common_message_buffer);
        va_end(ap2);
    }
    va_end(ap);
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
