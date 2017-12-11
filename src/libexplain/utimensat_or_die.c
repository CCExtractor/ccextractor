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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/option.h>
#include <libexplain/output.h>
#include <libexplain/utimensat.h>


void
explain_utimensat_or_die(int fildes, const char *pathname,
    const struct timespec *data, int flags)
{
    if (explain_utimensat_on_error(fildes, pathname, data, flags) < 0)
    {
        explain_output_exit_failure();
    }
}


#ifndef HAVE_UTIMENSAT

static int
utimensat(int fildes, const char *pathname, const struct timespec *data,
    int flags)
{
    (void)fildes;
    (void)pathname;
    (void)data;
    (void)flags;
    errno = ENOSYS;
    return -1;
}

#endif


int
explain_utimensat_on_error(int fildes, const char *pathname, const struct
    timespec *data, int flags)
{
    int             result;

    result = utimensat(fildes, pathname, data, flags);

#ifdef __linux__
    /*
     * Work around a kernel bug:
     * http://bugzilla.redhat.com/442352
     * http://bugzilla.redhat.com/449910
     * It appears that utimensat can mistakenly return 280 rather
     * than -1 upon ENOSYS failure.
     */
    if (result > 0 && errno == ENOSYS)
        result = -1;
#endif

    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_message(explain_errno_utimensat(hold_errno, fildes,
            pathname, data, flags));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
