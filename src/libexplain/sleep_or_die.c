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
#include <libexplain/ac/unistd.h>

#include <libexplain/output.h>
#include <libexplain/sleep.h>


unsigned int
explain_sleep_or_die(unsigned int seconds)
{
    int             hold_errno;
    unsigned int    result;

    hold_errno = errno;
    errno = 0;
    result = explain_sleep_on_error(seconds);
    if (errno != 0)
    {
        explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


unsigned int
explain_sleep_on_error(unsigned int seconds)
{
    int             hold_errno;
    unsigned int    result;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_SLEEP
    result = sleep(seconds);
#else
# if HAVE_NANOSLEEP
    {
        struct timespec req;
        struct timespec rem;
        req.tv_sec = seonds;
        req.tv_nsec = 0;
        explain_nanosleep_on_error(&req, &rem);
        return rem.tv_sec;
    }
# else
    errno = ENOSYS;
    result = -1;
# endif
#endif
    if (errno != 0)
    {
        hold_errno = errno;
        explain_output_error("%s", explain_errno_sleep(hold_errno, seconds));
        errno = hold_errno;
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
