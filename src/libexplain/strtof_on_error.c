/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
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

#include <libexplain/strtof.h>
#include <libexplain/output.h>


float
explain_strtof_on_error(const char *nptr, char **endptr)
{
    int             hold_errno;
    char            *dummy;
    float           result;

    hold_errno = errno;
    dummy = 0;
    errno = 0;
    result = explain_ac_strtof(nptr, endptr ? endptr : &dummy);
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
            explain_errno_strtof(hold_errno, nptr, endptr)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
