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
#include <libexplain/ac/stdlib.h>

#include <libexplain/realloc.h>
#include <libexplain/output.h>


void *
explain_realloc_on_error(void *ptr, size_t size)
{
    int             hold_errno;
    size_t          ok_size;
    void            *result;

    /*
     * Common mis-implementations of the posix standard:
     * Some realloc implementations can't cope with a size of zero.
     * Some realloc implementations can't cope with a ptr of NULL.
     */
    hold_errno = errno;
    errno = 0;
    ok_size = size ? size : 1;
    result = ptr ? realloc(ptr, ok_size) : malloc(ok_size);
    if (!result)
    {
        hold_errno = errno;
        if (hold_errno == 0)
            hold_errno = ENOMEM;
        explain_output_error("%s", explain_errno_realloc(hold_errno, ptr,
            size));
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
