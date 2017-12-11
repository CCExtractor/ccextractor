/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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
#include <libexplain/vasprintf.h>


int
explain_vasprintf_or_die(char **data, const char *format, va_list ap)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
    result = explain_vasprintf_on_error(data, format, ap);
    if (result < 0 || errno)
    {
        explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


int
explain_vasprintf_on_error(char **data, const char *format, va_list ap)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_VASPRINTF
    result = vasprintf(data, format, ap);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0 || errno != 0)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_vasprintf(hold_errno, data, format, ap)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
