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
#include <libexplain/ac/time.h>

#include <libexplain/nanosleep.h>
#include <libexplain/output.h>


void
explain_nanosleep_or_die(const struct timespec *req, struct timespec *rem)
{
    if (explain_nanosleep_on_error(req, rem) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_nanosleep_on_error(const struct timespec *req, struct timespec *rem)
{
    int             hold_errno;
    int             result;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_NANOSLEEP
    result = nanosleep(req, rem);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        /*
         * Linux-2.6.8.1's nanosleep returns -1, but doesn't set errno
         * when resumed after being suspended.  Earlier versions would
         * set errno to EINTR. nanosleep from linux-2.6.10, as well as
         * implementations by (all?) other vendors, doesn't return -1 in
         * that case; either it continues sleeping (if time remains. or
         * it returns zero (if the wake-up time has passed).
         */
        hold_errno = errno ? errno : EINTR;
        explain_output_error
        (
            "%s",
            explain_errno_nanosleep(hold_errno, req, rem)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
