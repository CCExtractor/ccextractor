/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/stdio.h>

#include <libexplain/fileinfo.h>
#include <libexplain/filename.h>


int
explain_filename_from_stream(FILE *stream, char *data, size_t data_size)
{
    return explain_filename_from_fildes(fileno(stream), data, data_size);
}


int
explain_filename_from_fildes(int fildes, char *data, size_t data_size)
{
    int ok = explain_fileinfo_self_fd_n(fildes, data, data_size);
    return (ok ? 0 : -1);
}


/* vim: set ts=8 sw=4 et : */
