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

#include <libexplain/gethostid.h>
#include <libexplain/output.h>


long
explain_gethostid_or_die(void)
{
    long            result;

    errno = 0;
    result = explain_gethostid_on_error();
    if (errno != 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


long
explain_gethostid_on_error(void)
{
    long            result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_GETHOSTID
    result = gethostid();
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (errno != 0)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_gethostid(hold_errno)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
