/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/ac/time.h>

#include <libexplain/output.h>
#include <libexplain/time.h>


time_t
explain_time_or_die(time_t *t)
{
    time_t          result;

    result = explain_time_on_error(t);
    if (result == (time_t)-1)
    {
        explain_output_exit_failure();
    }
    return result;
}

/* vim: set ts=8 sw=4 et : */
