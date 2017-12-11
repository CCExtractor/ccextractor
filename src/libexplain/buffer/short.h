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

#ifndef LIBEXPLAIN_BUFFER_SHORT_H
#define LIBEXPLAIN_BUFFER_SHORT_H

#include <libexplain/ac/stddef.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_short function may be used to print a
  * representation of a short integer value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The short value to be printed.
  */
void explain_buffer_short(explain_string_buffer_t *sb, short value);

/**
  * The explain_buffer_short function may be used to print a
  * representation of an unsigned short integer value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The short value to be printed.
  */
void explain_buffer_ushort(explain_string_buffer_t *sb, unsigned short value);

/**
  * The explain_buffer_short function may be used to print a
  * representation of a pointer to a short integer value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The short value to be printed.
  * @param data_size
  *     The length of the array.
  */
void explain_buffer_short_star(explain_string_buffer_t *sb, const short *data,
    size_t data_size);

/**
  * The explain_buffer_short function may be used to print a
  * representation of a pointer to anunsigned short integer value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The short value to be printed.
  * @param data_size
  *     The length of the array.
  */
void explain_buffer_ushort_star(explain_string_buffer_t *sb,
    const unsigned short *data, size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_SHORT_H */
/* vim: set ts=8 sw=4 et : */
