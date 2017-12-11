/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2012, 2013 Peter Miller
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
#include <libexplain/ac/sys/time.h>

#include <libexplain/futimes.h>
#include <libexplain/output.h>


int
explain_futimes_on_error(int fildes, const struct timeval *tv)
{
    int             result;

#ifdef HAVE_FUTIMES
    result = futimes(fildes, tv);
#ifdef __linux__
    if (result > 0)
    {
        /* Work around a kernel bug:
         * http://bugzilla.redhat.com/442352
         * http://bugzilla.redhat.com/449910
         * It appears that utimensat can mistakenly return 280 rather
         * than -1 upon ENOSYS failure.
         */
        errno = ENOSYS;
        result = -1;
    }
#endif /* __linux__ */
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_futimes(hold_errno, fildes,
            tv));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
