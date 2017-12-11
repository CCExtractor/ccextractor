/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
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

#include <libexplain/ac/dirent.h>
#include <libexplain/ac/errno.h>

#include <libexplain/fdopendir.h>
#include <libexplain/output.h>


#ifndef HAVE_FDOPENDIR

static DIR *
fdopendir(int fildes)
{
    (void)fildes;
    errno = ENOSYS;
    return NULL;
}

#endif


DIR *
explain_fdopendir_on_error(int fildes)
{
    DIR             *result;

    result = fdopendir(fildes);
    if (!result)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_fdopendir(hold_errno,
            fildes));
        errno = hold_errno;
    }
    return result;
}


DIR *
explain_fdopendir_or_die(int fildes)
{
    DIR             *result;

    result = explain_fdopendir_on_error(fildes);
    if (!result)
    {
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
