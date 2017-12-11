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

#ifndef LIBEXPLAIN_BUFFER_INT8_H
#define LIBEXPLAIN_BUFFER_INT8_H

#include <libexplain/ac/stddef.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_int8 function may be used
  * to print a representation of an int8 (char) value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The value to be printed.
  */
void explain_buffer_int8(explain_string_buffer_t *sb, signed char data);

/**
  * The explain_buffer_int8_star function may be used
  * to print a representation of an int8 (char) value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The value to be printed.
  * @param data_size
  *     The size of the array value to be printed.
  */
void explain_buffer_int8_star(explain_string_buffer_t *sb,
    const signed char *data, size_t data_size);

/**
  * The explain_buffer_uint8 function may be used
  * to print a representation of an uint8 (unsigned char) value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The value to be printed.
  */
void explain_buffer_uint8(explain_string_buffer_t *sb, unsigned char data);

/**
  * The explain_buffer_uint8_star function may be used
  * to print a representation of an uint8 (unsigned char) value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The value to be printed.
  * @param data_size
  *     The size of the value array to be printed.
  */
void explain_buffer_uint8_star(explain_string_buffer_t *sb,
    const unsigned char *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_INT8_H */
/* vim: set ts=8 sw=4 et : */
