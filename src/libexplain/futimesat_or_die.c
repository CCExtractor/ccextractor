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
#include <libexplain/ac/sys/time.h>

#include <libexplain/futimesat.h>
#include <libexplain/output.h>


void
explain_futimesat_or_die(int fildes, const char *pathname, const struct timeval
    *data)
{
    if (explain_futimesat_on_error(fildes, pathname, data) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_futimesat_on_error(int fildes, const char *pathname, const struct
    timeval *data)
{
    int             result;

#ifdef HAVE_FUTIMESAT
    result = futimesat(fildes, pathname, data);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_futimesat(hold_errno, fildes, pathname, data)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
