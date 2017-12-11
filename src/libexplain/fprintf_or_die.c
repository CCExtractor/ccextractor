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
#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/errno/fprintf.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/fprintf.h>
#include <libexplain/output.h>
#include <libexplain/string_buffer.h>


int
explain_fprintf_or_die(FILE *fp, const char *format, ...)
{
    int             hold_errno;
    va_list         ap;
    va_list         ap2;
    int             result;

    va_start(ap, format);

    /*
     * We may need a second copy, just in case the call fails.  We can't
     * call va_start again, that isn't the way this works.  And we can't
     * do a simple assignment, because there are some funky compilers
     * out there, and va_list could be some serious coding.
     *
     * <rant>
     * The va_copy functionality was introduced in ANSI C in 1999.  If
     * your compiler doesn't have it, raise a bug with your vendor.  You
     * should also SERIOUSLY consider upgrading to gcc, which has been
     * ANSI C 1999 compliant since 1999.
     * </rant>
     */
    va_copy(ap2, ap);

    hold_errno = errno;
    errno = EINVAL;
    result = vfprintf(fp, format, ap);
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
        explain_buffer_errno_fprintf(&sb, hold_errno, fp, format, ap2);
        explain_output_error("%s", explain_common_message_buffer);
        explain_output_exit_failure();
    }
    va_end(ap2); /* yes, both of them */
    va_end(ap);
    errno = hold_errno;
    return result;
}


int
explain_fprintf_on_error(FILE *fp, const char *format, ...)
{
    int             hold_errno;
    int             result;
    va_list         ap;
    va_list         ap2;

    va_start(ap, format);
    va_copy(ap2, ap); /* see comments in explain_fprintf_or_die */

    hold_errno = errno;
    errno = EINVAL;
    result = vfprintf(fp, format, ap);
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
        explain_buffer_errno_fprintf(&sb, hold_errno, fp, format, ap2);
        explain_output_error("%s", explain_common_message_buffer);
    }
    va_end(ap2); /* yes, both of them */
    va_end(ap);
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
