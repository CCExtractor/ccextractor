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

#include <libexplain/ac/unistd.h>

#include <libexplain/output.h>
#include <libexplain/pwrite.h>


ssize_t
explain_pwrite_or_die(int fildes, const void *data, size_t data_size, off_t
    offset)
{
    ssize_t         result;

    result = explain_pwrite_on_error(fildes, data, data_size, offset);
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}

/* vim: set ts=8 sw=4 et : */
