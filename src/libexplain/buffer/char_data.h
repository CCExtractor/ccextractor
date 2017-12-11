/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_CHAR_DATA_H
#define LIBEXPLAIN_BUFFER_CHAR_DATA_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_char_data function may be used to print an array
  * of char data, but it is <b>not</b> a string.
  *
  * @param sb
  *    The string buffer to print into.
  * @param data
  *    The base of the array to be printed
  * @param data_size
  *    The size of the array to be printed, int bytes.
  */
void explain_buffer_char_data(explain_string_buffer_t *sb,
    const void *data, size_t data_size);

/**
  * The explain_buffer_char_data_quoted function may be used to print an
  * array of char data, and it is thought to be a NUL-terminated string.
  *
  * @param sb
  *    The string buffer to print into.
  * @param data
  *    The base of the array to be printed
  * @param data_size
  *    The size of the array to be printed, int bytes.
  */
void explain_buffer_char_data_quoted(explain_string_buffer_t *sb,
    const void *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_CHAR_DATA_H */
/* vim: set ts=8 sw=4 et : */
