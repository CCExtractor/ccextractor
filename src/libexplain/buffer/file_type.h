/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_BUFFER_FILE_TYPE_H
#define LIBEXPLAIN_BUFFER_FILE_TYPE_H

struct explain_string_buffer_t; /* forward */

#define FILE_TYPE_BUFFER_SIZE_MIN 100

/**
  * The explain_buffer_file_type function may be used to turn a file
  * type from struct stat::st_mode into a human readable string.
  *
  * @param sb
  *    The string buffer to write the file type name to.
  * @param mode
  *    The file type, exactly as seen in struct stat::st_mode.
  */
void explain_buffer_file_type(struct explain_string_buffer_t *sb,
    int mode);

struct stat; /* forward */

/**
  * The explain_buffer_file_type function may be used to turn a file
  * type from struct stat::st_mode into a human readable string.
  *
  * @param sb
  *    The string buffer to write the file type name to.
  * @param st
  *    The file meta-data, exactly as returned by stat(2) et al.
  */
void explain_buffer_file_type_st(struct explain_string_buffer_t *sb,
    const struct stat *st);

#endif /* LIBEXPLAIN_BUFFER_FILE_TYPE_H */
/* vim: set ts=8 sw=4 et : */
