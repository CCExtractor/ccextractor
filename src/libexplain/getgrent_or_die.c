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
#include <libexplain/ac/grp.h>

#include <libexplain/getgrent.h>
#include <libexplain/output.h>


struct group *
explain_getgrent_or_die(void)
{
    struct group    *result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
    result = explain_getgrent_on_error();
    if (!result)
    {
        /*
         * The getgrent() function returns a pointer to a group
         * structure, or NULL if there are no more entries or an error
         * occurs.
         *
         * Upon error, errno may be set.  If one wants to check errno
         * after the call, it should be set to zero before the call.
         */
        if (errno != 0)
            explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


struct group *
explain_getgrent_on_error(void)
{
    struct group    *result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_GETGRENT
    result = getgrent();
#else
    errno = ENOSYS;
    result = 0;
#endif
    if (!result)
    {
        /*
         * The getgrent() function returns a pointer to a group
         * structure, or NULL if there are no more entries or an error
         * occurs.
         *
         * Upon error, errno may be set.  If one wants to check errno
         * after the call, it should be set to zero before the call.
         */
        if (errno != 0)
        {
            hold_errno = errno;
            explain_output_error("%s", explain_errno_getgrent(hold_errno));
        }
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
