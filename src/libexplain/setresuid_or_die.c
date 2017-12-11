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
#include <libexplain/ac/unistd.h>

#include <libexplain/output.h>
#include <libexplain/setresuid.h>


void
explain_setresuid_or_die(uid_t ruid, uid_t euid, uid_t suid)
{
    if (explain_setresuid_on_error(ruid, euid, suid) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_setresuid_on_error(uid_t ruid, uid_t euid, uid_t suid)
{
    int             result;

#ifdef HAVE_SETRESUID
    result = setresuid(ruid, euid, suid);
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
            explain_errno_setresuid(hold_errno, ruid, euid, suid)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
