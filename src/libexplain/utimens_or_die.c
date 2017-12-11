/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h> /* for AT_FDCWD */
#include <libexplain/ac/sys/stat.h> /* for utimens and utimensat */

#include <libexplain/option.h>
#include <libexplain/output.h>
#include <libexplain/utimens.h>


void
explain_utimens_or_die(const char *pathname, const struct timespec *data)
{
    if (explain_utimens_on_error(pathname, data) < 0)
    {
        explain_output_exit_failure();
    }
}


#ifndef HAVE_UTIMENS

static int
utimens(char const *path, const struct timespec data[2])
{
#ifndef HAVE_UTIMENSAT
    (void)path;
    (void)data;
    errno = ENOSYS;
    return -1;
#else
    return utimensat(AT_FDCWD, path, data, 0);
#endif
}

#endif /* !HAVE_UTIMENS */


int
explain_utimens_on_error(const char *pathname, const struct timespec *data)
{
    int             result;

    result = utimens(pathname, data);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_message(explain_errno_utimens(hold_errno, pathname,
            data));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
