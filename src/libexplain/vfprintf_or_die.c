/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
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

#include <libexplain/output.h>
#include <libexplain/vfprintf.h>


void
explain_vfprintf_or_die(FILE *fp, const char *format, va_list ap)
{
    if (explain_vfprintf_on_error(fp, format, ap) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_vfprintf_on_error(FILE *fp, const char *format, va_list ap)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = EINVAL;
    result = vfprintf(fp, format, ap);
fprintf(stderr, "%s: %d: result = %d\n", __FILE__, __LINE__, result);
    if (result < 0)
    {
        hold_errno = errno;
        explain_output_message
        (
            explain_errno_vfprintf(hold_errno, fp, format, ap)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
