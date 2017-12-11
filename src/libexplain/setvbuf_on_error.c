/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2012, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/setvbuf.h>
#include <libexplain/output.h>


int
explain_setvbuf_on_error(FILE *fp, char *data, int mode, size_t size)
{
    int             hold_errno;
    int             result;

    hold_errno = errno;
    errno = 0;
    result = setvbuf(fp, data, mode, size);
    if (result < 0)
    {
        hold_errno = errno;
        explain_output_message
        (
            explain_errno_setvbuf(hold_errno, fp, data, mode, size)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
