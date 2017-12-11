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

#include <libexplain/linkat.h>
#include <libexplain/output.h>


void
explain_linkat_or_die(int old_fildes, const char *old_path, int new_fildes,
    const char *new_path, int flags)
{
    if (explain_linkat_on_error(old_fildes, old_path, new_fildes, new_path,
        flags) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_linkat_on_error(int old_fildes, const char *old_path, int new_fildes,
    const char *new_path, int flags)
{
    int             result;

#ifdef HAVE_LINKAT
    result = linkat(old_fildes, old_path, new_fildes, new_path, flags);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_linkat(hold_errno, old_fildes, old_path, new_fildes,
                new_path, flags)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
