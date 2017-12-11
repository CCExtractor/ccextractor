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
#include <libexplain/ac/stdlib.h>

#include <libexplain/calloc.h>
#include <libexplain/output.h>


void *
explain_calloc_or_die(size_t nmemb, size_t size)
{
    void            *result;

    result = explain_calloc_on_error(nmemb, size);
    if (!result)
    {
        explain_output_exit_failure();
    }
    return result;
}


void *
explain_calloc_on_error(size_t nmemb, size_t size)
{
    void            *result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
    result = calloc(nmemb, size);
    if (!result)
    {
        /* this is deliberately conservative */
        size_t nmemb_avail = (((size_t)-1) / 3 * 2 / size);
        hold_errno = (errno ? errno : (nmemb_avail < nmemb ? EINVAL : ENOMEM));
        explain_output_error
        (
            "%s",
            explain_errno_calloc(hold_errno, nmemb, size)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
