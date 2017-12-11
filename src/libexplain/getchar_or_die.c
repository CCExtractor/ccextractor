/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012-2014 Peter Miller
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

#include <libexplain/ac/stdio.h>

#include <libexplain/getchar.h>
#include <libexplain/output.h>


#if !(__GNUC__ >= 3 || defined(__clang__))
static
#endif

void
explain_getchar_or_die_failed(void)
{
    explain_output_error("%s", explain_getchar());
    explain_output_exit_failure();
}


#if !(__GNUC__ >= 3 || defined(__clang__))

int
explain_getchar_or_die(void)
{
    int             result;

    result = getchar();
    if (result == EOF && ferror(stdin))
        explain_getchar_or_die_failed();
    return result;
}

#endif


/* vim: set ts=8 sw=4 et : */
