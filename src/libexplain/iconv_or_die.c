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

#include <libexplain/iconv.h>
#include <libexplain/output.h>


#define ICONV_FAIL ((size_t)(-1))


size_t
explain_iconv_or_die(iconv_t cd, char **inbuf, size_t *inbytesleft, char
    **outbuf, size_t *outbytesleft)
{
    int             hold_errno;
    size_t          result;

    hold_errno = errno;
    errno = 0;
    result = explain_iconv_on_error(cd, inbuf, inbytesleft, outbuf,
        outbytesleft);
    if (result == ICONV_FAIL || errno == 0)
    {
        explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


size_t
explain_iconv_on_error(iconv_t cd, char **inbuf, size_t *inbytesleft, char
    **outbuf, size_t *outbytesleft)
{
    size_t          result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_ICONV
    result = iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
#else
    errno = ENOSYS;
    result = ICONV_FAIL;
#endif
    if (result == ICONV_FAIL)
    {
        barf:
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_iconv(hold_errno, cd, inbuf, inbytesleft, outbuf,
                outbytesleft)
        );
    }
    else if (errno != 0)
    {
        goto barf;
    }
    else
    {
        /* "irreversible" means broken */
        errno = EILSEQ;
        result = ICONV_FAIL;

        /*
         * From the eglibc sources::iconv/iconv.c
         *
         * "Irix iconv() inserts a NUL byte if it cannot convert.
         * NetBSD iconv() inserts a question mark if it cannot convert.
         * Only GNU libiconv and GNU libc are known to prefer to fail
         * rather than doing a lossy conversion."
         */
        goto barf;
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
