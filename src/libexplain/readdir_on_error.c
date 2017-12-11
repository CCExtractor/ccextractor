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
#include <libexplain/ac/dirent.h>

#include <libexplain/readdir.h>
#include <libexplain/output.h>


struct dirent *
explain_readdir_on_error(DIR *dir)
{
    int             hold_errno;
    struct dirent   *result;

    hold_errno = errno;
    errno = 0;
    result = readdir(dir);
    if (!result && errno != 0)
    {
        hold_errno = errno;
        explain_output_error("%s", explain_errno_readdir(hold_errno, dir));
    }
    errno = hold_errno;
    return result;
}

/* vim: set ts=8 sw=4 et : */
