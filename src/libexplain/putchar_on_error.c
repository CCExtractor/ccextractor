/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2014 Peter Miller
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

#include <libexplain/putchar.h>
#include <libexplain/output.h>


void
explain_putchar_on_error_failed(int c)
{
    int             hold_errno;

    hold_errno = errno;
    explain_output_error("%s", explain_errno_putchar(hold_errno, c));
    errno = hold_errno;
}


#if !(defined(__GNUC__) || defined(__clang__))

int
explain_putchar_on_error(int c)
{
    int             result;

    result = putchar(c);
    if (result == EOF)
        explain_putchar_on_error_failed(c);
    return result;
}

#endif

/* vim: set ts=8 sw=4 et : */
