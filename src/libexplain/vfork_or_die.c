/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/ac/sys/types.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/output.h>
#include <libexplain/vfork.h>


pid_t
explain_vfork_or_die(void)
{
    pid_t           result;

    result = explain_vfork_on_error();
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
