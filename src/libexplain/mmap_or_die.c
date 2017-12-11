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
#include <libexplain/ac/sys/mman.h>

#include <libexplain/mmap.h>
#include <libexplain/output.h>


void *
explain_mmap_or_die(void *data, size_t data_size, int prot, int flags, int
    fildes, off_t offset)
{
    int             hold_errno;
    void            *result;

    hold_errno = errno;
    errno = 0;
    result = explain_mmap_on_error(data, data_size, prot, flags, fildes,
        offset);
    if
    (
        result == (void *)(-1)
    ||
        (
            !result
        &&
            (data != 0 || !(flags & MAP_FIXED) || errno)
        )
    )
    {
        explain_output_exit_failure();
    }
    errno = hold_errno;
    return result;
}


void *
explain_mmap_on_error(void *data, size_t data_size, int prot, int flags, int
    fildes, off_t offset)
{
    int             hold_errno;
    void            *result;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_MMAP
    result = mmap(data, data_size, prot, flags, fildes, offset);
#else
    errno = ENOSYS;
    result = 0;
#endif
    if
    (
        !result
    &&
        (data != 0 || !(flags & MAP_FIXED) || errno)
    )
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_mmap
            (
                hold_errno,
                data,
                data_size,
                prot,
                flags,
                fildes,
                offset
            )
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
