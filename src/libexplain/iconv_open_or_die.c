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
#include <libexplain/ac/iconv.h>

#include <libexplain/iconv_open.h>
#include <libexplain/output.h>


#define FAIL ((iconv_t)(-1))


iconv_t
explain_iconv_open_or_die(const char *tocode, const char *fromcode)
{
    iconv_t         result;

    result = explain_iconv_open_on_error(tocode, fromcode);
    if (result == FAIL)
    {
        explain_output_exit_failure();
    }
    return result;
}


iconv_t
explain_iconv_open_on_error(const char *tocode, const char *fromcode)
{
    iconv_t         result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_ICONV_OPEN
    result = iconv_open(tocode, fromcode);
#else
    errno = ENOSYS;
    result = FAIL;
#endif
    if (result == FAIL)
    {
        hold_errno = errno ? errno : EINVAL;
        explain_output_error
        (
            "%s",
            explain_errno_iconv_open(hold_errno, tocode, fromcode)
        );
        errno = hold_errno;
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
