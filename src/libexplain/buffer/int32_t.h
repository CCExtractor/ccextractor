/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_INT32_T_H
#define LIBEXPLAIN_BUFFER_INT32_T_H

#include <libexplain/ac/stdint.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_int32_t function may be used
  * to print a representation of a int32_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int32_t value to be printed.
  */
void explain_buffer_int32_t(explain_string_buffer_t *sb, int32_t data);

/**
  * The explain_buffer_int32_array function may be used
  * to print a representation of an array of int32_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int32_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_int32_array(explain_string_buffer_t *sb,
    const int32_t *data, size_t data_size);

/**
  * The explain_int32_array_all_zero function may be used to
  * test an int32_t array, to see if all the values are zero.
  *
  * @param data
  *     The int32_t array to be tested.
  * @param data_size
  *     The number of elements in the array to be tested.
  */
int explain_int32_array_all_zero(const int32_t *data, size_t data_size);

/**
  * The explain_buffer_uint32_t function may be used
  * to print a representation of a uint32_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint32_t value to be printed.
  */
void explain_buffer_uint32_t(explain_string_buffer_t *sb, uint32_t data);

/**
  * The explain_buffer_uint32_array function may be used
  * to print a representation of an array of uint32_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint32_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_uint32_array(explain_string_buffer_t *sb,
    const uint32_t *data, size_t data_size);

/**
  * The explain_uint32_array_all_zero function may be used to
  * test an uint32_t array, to see if all the values are zero.
  *
  * @param data
  *     The uint32_t array to be tested.
  * @param data_size
  *     The number of elements in the array to be tested.
  */
int explain_uint32_array_all_zero(const uint32_t *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_INT32_T_H */
/* vim: set ts=8 sw=4 et : */
