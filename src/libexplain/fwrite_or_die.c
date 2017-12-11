/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
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
#include <libexplain/ac/stdio.h>

#include <libexplain/fwrite.h>
#include <libexplain/output.h>


size_t
explain_fwrite_on_error(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    size_t          result;

    result = fwrite(ptr, size, nmemb, fp);
    if (result == 0 && ferror(fp))
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_fwrite(hold_errno, ptr,
            size, nmemb, fp));
        errno = hold_errno;
    }
    return result;
}


size_t
explain_fwrite_or_die(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    size_t          result;

    result = explain_fwrite_on_error(ptr, size, nmemb, fp);
    if (result < nmemb)
    {
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
