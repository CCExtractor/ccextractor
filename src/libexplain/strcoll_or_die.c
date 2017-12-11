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
#include <libexplain/ac/string.h>

#include <libexplain/output.h>
#include <libexplain/strcoll.h>


int
explain_strcoll_or_die(const char *s1, const char *s2)
{
    int             result;

    errno = 0;
    result = explain_strcoll_on_error(s1, s2);
    if (errno != 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


int
explain_strcoll_on_error(const char *s1, const char *s2)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_STRCOLL
    result = strcoll(s1, s2);
#else
    errno = ENOSYS;
    result = 0; /* result of string comparison, can be any int value */
#endif
    if (errno != 0)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_strcoll(hold_errno, s1, s2)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
