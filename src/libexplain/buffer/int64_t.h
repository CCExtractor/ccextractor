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

#ifndef LIBEXPLAIN_BUFFER_INT64_T_H
#define LIBEXPLAIN_BUFFER_INT64_T_H

#include <libexplain/ac/stdint.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_int64_t function may be used
  * to print a representation of a int64_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int64_t value to be printed.
  */
void explain_buffer_int64_t(explain_string_buffer_t *sb, int64_t data);

/**
  * The explain_buffer_int64_array function may be used
  * to print a representation of an array of int64_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int64_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_int64_array(explain_string_buffer_t *sb,
    const int64_t *data, size_t data_size);

/**
  * The explain_buffer_uint64_t function may be used
  * to print a representation of a uint64_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint64_t value to be printed.
  */
void explain_buffer_uint64_t(explain_string_buffer_t *sb, uint64_t data);

/**
  * The explain_buffer_uint64_array function may be used
  * to print a representation of an array of uint64_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint64_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_uint64_array(explain_string_buffer_t *sb,
    const uint64_t *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_INT64_T_H */
/* vim: set ts=8 sw=4 et : */
