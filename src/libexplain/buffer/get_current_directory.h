/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_GET_CURRENT_DIRECTORY_H
#define LIBEXPLAIN_BUFFER_GET_CURRENT_DIRECTORY_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_get_current_directory function may be used to
  * obtain the path of the current directory, printing errors as and when
  * they are found.
  *
  * @param sb
  *    The string buffer to print into.
  * @param data
  *    The buffer to place the generated path into, once found
  * @param data_size
  *    The siz eof the buffer, in bytes.
  * @returns
  *    int; 0 in success (path found, no errors), or -1 when path not
  *    found and message already printed.
  */
int explain_buffer_get_current_directory(explain_string_buffer_t *sb,
    char *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_GET_CURRENT_DIRECTORY_H */
/* vim: set ts=8 sw=4 et : */
