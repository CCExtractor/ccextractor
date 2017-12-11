/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/fopen.h>
#include <libexplain/output.h>


FILE *
explain_fopen_on_error(const char *pathname, const char *flags)
{
    FILE            *result;

    result = fopen(pathname, flags);
    if (!result)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_fopen(hold_errno, pathname,
            flags));
        errno = hold_errno;
    }
    return result;
}


FILE *
explain_fopen_or_die(const char *pathname, const char *flags)
{
    FILE            *fp;

    fp = explain_fopen_on_error(pathname, flags);
    if (!fp)
    {
        explain_output_exit_failure();
    }
    return fp;
}


/* vim: set ts=8 sw=4 et : */
