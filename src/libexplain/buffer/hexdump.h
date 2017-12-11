/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_HEXDUMP_H
#define LIBEXPLAIN_BUFFER_HEXDUMP_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_hexdump function may be used to dump data in
  * hex form to the given string buffer.
  *
  * @param sb
  *    The string buffer to print into.
  * @param data
  *    The data to dump
  * @param data_size
  *    The size of the data to dump
  */
void explain_buffer_hexdump(explain_string_buffer_t *sb, const void *data,
    size_t data_size);

/**
  * The explain_buffer_hexdump_array function may be used to dump data in
  * hex form to the given string buffer.
  *
  * @param sb
  *    The string buffer to print into.
  * @param data
  *    The data to dump
  * @param data_size
  *    The size of the data to dump
  */
void explain_buffer_hexdump_array(explain_string_buffer_t *sb, const void *data,
    size_t data_size);

void explain_buffer_mostly_text(explain_string_buffer_t *sb, const void *data,
    size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_HEXDUMP_H */
/* vim: set ts=8 sw=4 et : */
