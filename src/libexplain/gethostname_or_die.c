/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/gethostname.h>
#include <libexplain/output.h>


void
explain_gethostname_or_die(char *data, size_t data_size)
{
    if (explain_gethostname_on_error(data, data_size) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_gethostname_on_error(char *data, size_t data_size)
{
    int             result;

#ifdef HAVE_GETHOSTNAME
    result = gethostname(data, data_size);
    if (result >= 0 && data_size > 0)
    {
        /*
         * Use data_size-1 here rather than SIZE to work around the bug
         * in SunOS 5.5's gethostname whereby it NUL-terminates HOSTNAME
         * even when the name is as long as the supplied buffer.
         *
         * provenance: coreutils::xgethostname
         *
         * POSIX.1-2001 says that if such truncation occurs, then
         * it is unspecified whether the returned buffer includes a
         * terminating NUL byte.
         */
        data[data_size - 1] = '\0';
    }
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_gethostname(hold_errno,
            data, data_size));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
