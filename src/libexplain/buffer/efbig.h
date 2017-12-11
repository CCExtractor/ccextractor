/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_EFBIG_H
#define LIBEXPLAIN_BUFFER_EFBIG_H

#include <libexplain/string_buffer.h>

/**
  * The explain_get_max_file_size_by_pathname function si sued to
  * get the maximum file size, for the file system on which the file
  * resides.
  *
  * @param path
  *     The path indicating the file system.
  * @returns
  *     maximum file size
  */
unsigned long long explain_get_max_file_size_by_pathname(const char *path);

/**
  * The explain_buffer_efbig function may be used to insert the
  * maximum file size into the giben string buffer.
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The pathname of the problem file, so pathconf can be used.
  * @param length
  *    The length of offending file
  * @param length_caption
  *    The name of the parameter of the length of offending file
  */
void explain_buffer_efbig(explain_string_buffer_t *sb,
    const char *pathname, unsigned long long length,
    const char *length_caption);

/**
  * The explain_get_max_file_size_by_fildes function is used to
  * get the maximum file size, for the file system on which the file
  * resides.
  *
  * @param fildes
  *     The file descriptor indicating the file system.
  * @returns
  *     maximum file size
  */
unsigned long long explain_get_max_file_size_by_fildes(int fildes);

/**
  * The explain_buffer_efbig_fildes function may be used to insert the
  * maximum file size into the given string buffer.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The filedescriptor of the problem file, so fpathconf can be used.
  * @param length
  *    The length of offending file
  * @param length_caption
  *    The name of the parameter of the length of offending file
  */
void explain_buffer_efbig_fildes(explain_string_buffer_t *sb, int fildes,
    unsigned long long length, const char *length_caption);

#endif /* LIBEXPLAIN_BUFFER_EFBIG_H */
/* vim: set ts=8 sw=4 et : */
