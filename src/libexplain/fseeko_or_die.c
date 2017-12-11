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
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/sys/types.h>

#include <libexplain/fseeko.h>
#include <libexplain/output.h>


int
explain_fseeko_on_error(FILE *fp, off_t offset, int whence)
{
    int             result;

#ifdef HAVE_FSEEKO
    result = fseeko(fp, offset, whence);
#else
    result = -1;
    errno = ENOSYS;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_fseeko(hold_errno, fp,
            offset, whence));
        errno = hold_errno;
    }
    return result;
}


void
explain_fseeko_or_die(FILE *fp, off_t offset, int whence)
{
    if (explain_fseeko_on_error(fp, offset, whence) < 0)
    {
        explain_output_exit_failure();
    }
}


/* vim: set ts=8 sw=4 et : */
