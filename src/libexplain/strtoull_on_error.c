/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdlib.h>

#include <libexplain/strtoull.h>
#include <libexplain/output.h>


unsigned long long
explain_strtoull_on_error(const char *nptr, char **endptr, int base)
{
    int             hold_errno;
    char            *dummy;
    unsigned long long result;

    dummy = 0;
    hold_errno = errno;
    errno = 0;
    result = strtoull(nptr, endptr ? endptr : &dummy, base);
    if (errno == 0 && (endptr ? *endptr : dummy) == nptr)
    {
        /*
         * Some implementations do this natively.
         * Fake it on those that don't.
         */
        errno = EINVAL;
    }
    if (errno != 0)
    {
        hold_errno = errno;
        explain_output_message
        (
            explain_errno_strtoull(hold_errno, nptr, endptr, base)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
