/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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
#include <libexplain/ac/sys/mount.h>

#include <libexplain/mount.h>
#include <libexplain/output.h>


void
explain_mount_or_die(const char *source, const char *target, const char
    *file_systems_type, unsigned long flags, const void *data)
{
    if (explain_mount_on_error(source, target, file_systems_type, flags, data) <
        0)
    {
        explain_output_exit_failure();
    }
}


int
explain_mount_on_error(const char *source, const char *target, const char
    *file_systems_type, unsigned long flags, const void *data)
{
    int             result;

#ifdef HAVE_MOUNT
# ifdef __FreeBSD__
    (void)file_systems_type;
    result = mount(source, target, flags, data);
#  else
    result = mount(source, target, file_systems_type, flags, data);
#  endif
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
            explain_errno_mount(hold_errno, source, target, file_systems_type,
                flags, data)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
