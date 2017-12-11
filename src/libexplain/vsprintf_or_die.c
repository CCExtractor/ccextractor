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
#include <libexplain/vsprintf.h>


#undef vsprintf


int
explain_vsprintf_or_die(char *data, const char *format, va_list ap)
{
    int             result;

    result = explain_vsprintf_on_error(data, format, ap);
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


int
explain_vsprintf_on_error(char *data, const char *format, va_list ap)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = EINVAL;
    result = vsprintf(data, format, ap);
    if (result < 0)
    {
        hold_errno = errno;
        explain_output_message
        (
            explain_errno_vsprintf(hold_errno, data, format, ap)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
