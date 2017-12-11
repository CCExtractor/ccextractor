/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011-2013 Peter Miller
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

#include <libexplain/output.h>
#include <libexplain/realpath.h>


char *
explain_realpath_or_die(const char *pathname, char *resolved_pathname)
{
    char            *result;

    result = explain_realpath_on_error(pathname, resolved_pathname);
    if (!result)
    {
        explain_output_exit_failure();
    }
    return result;
}


char *
explain_realpath_on_error(const char *pathname, char *resolved_pathname)
{
    char            *result;

    result = realpath(pathname, resolved_pathname);
    if (!result)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_realpath(hold_errno, pathname,
            resolved_pathname));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
