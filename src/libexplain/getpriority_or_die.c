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
#include <libexplain/ac/sys/resource.h>

#include <libexplain/getpriority.h>
#include <libexplain/output.h>


int
explain_getpriority_or_die(int which, int who)
{
    int             hold_errno;
    int             result;

    hold_errno = errno;
    errno = 0;
    /*
     * Since getpriority() can legitimately return the value -1, it is
     * necessary to clear the external variable errno prior to the call, then
     * check it afterward to determine if -1 is an error or a legitimate value.
     */
    result = explain_getpriority_on_error(which, who);
    if (errno != 0)
        hold_errno = errno;
    if (result == -1 && hold_errno != 0)
    {
        explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


int
explain_getpriority_on_error(int which, int who)
{
    int             hold_errno;
    int             result;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_GETPRIORITY
    result = getpriority(which, who);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result == -1 && errno != 0)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_getpriority(hold_errno, which, who)
        );
        errno = hold_errno;
        result = -1;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
