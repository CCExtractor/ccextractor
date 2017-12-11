/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_INT16_T_H
#define LIBEXPLAIN_BUFFER_INT16_T_H

#include <libexplain/ac/stdint.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_int16_t function may be used
  * to print a representation of a int16_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int16_t value to be printed.
  */
void explain_buffer_int16_t(explain_string_buffer_t *sb, int16_t data);

/**
  * The explain_buffer_int16_array function may be used
  * to print a representation of an array of int16_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The int16_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_int16_array(explain_string_buffer_t *sb,
    const int16_t *data, size_t data_size);

/**
  * The explain_buffer_uint16_t function may be used
  * to print a representation of a uint16_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint16_t value to be printed.
  */
void explain_buffer_uint16_t(explain_string_buffer_t *sb, uint16_t data);

/**
  * The explain_buffer_uint16_array function may be used
  * to print a representation of an array of uint16_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint16_t array to be printed.
  * @param data_size
  *     The number of elements in the array to be printed.
  */
void explain_buffer_uint16_array(explain_string_buffer_t *sb,
    const uint16_t *data, size_t data_size);

/**
  * The explain_buffer_uint16_array2 function may be used
  * to print a representation of a 2D array of uint16_t values.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The uint16_t array to be printed.
  * @param dim1_size
  *     The number of elements in the first dimension of the array to be
  *     printed.
  * @param dim2_size
  *     The number of elements in the second dimension of the array to be
  *     printed.
  */
void explain_buffer_uint16_array2(explain_string_buffer_t *sb,
    const uint16_t *data, size_t dim1_size, size_t dim2_size);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_INT16_T_H */
