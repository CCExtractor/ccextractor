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
#include <libexplain/ac/netdb.h>

#include <libexplain/gethostbyname.h>
#include <libexplain/output.h>


struct hostent *
explain_gethostbyname_or_die(const char *name)
{
    struct hostent  *result;

    result = explain_gethostbyname_on_error(name);
    if (!result)
    {
        explain_output_exit_failure();
    }
    return result;
}


struct hostent *
explain_gethostbyname_on_error(const char *name)
{
    struct hostent  *result;

#ifdef HAVE_GETHOSTBYNAME
    result = gethostbyname(name);
#else
    errno = ENOSYS;
#ifdef NETDB_INTERNAL
    h_error = NETDB_INTERNAL;
#else
    h_error = NO_RECOVERY;
#endif
    result = 0;
#endif
    if (!result)
    {
        int             hold_errno;
        int             hold_h_errno;

        hold_errno = errno;
        hold_h_errno = h_errno;
        explain_output_error
        (
            "%s",
            explain_errno_gethostbyname(hold_h_errno, name)
        );
        errno = hold_errno;
        h_errno = hold_h_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
