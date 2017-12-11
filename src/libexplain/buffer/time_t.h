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

#ifndef LIBEXPLAIN_BUFFER_TIME_T_H
#define LIBEXPLAIN_BUFFER_TIME_T_H

#include <libexplain/ac/time.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_time_t function may be used to print a
  * representation of a time_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The time_t value to be printed.
  */
void explain_buffer_time_t(explain_string_buffer_t *sb, time_t value);

/**
  * The explain_buffer_time_t function may be used to print a
  * representation of a time_t value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The time_t value to be printed.
  */
void explain_buffer_time_t_star(explain_string_buffer_t *sb,
    const time_t *data);

/**
  * The explain_parse_time_t_or_die function may be used to parse a
  * string containing a symbolic representation of a time_t value.
  *
  * @param text
  *     The text to be parsed to extract a permission mode value.
  * @param caption
  *     additional text to add to the start of the error message
  * @returns
  *     time_t value
  * @note
  *     If there is a parse error, a fatal error message is printed,
  *     and exit(EXIT_FAILURE) is called.  If there is an error, this
  *     function will not return.
  */
time_t explain_parse_time_t_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_TIME_T_H */
/* vim: set ts=8 sw=4 et : */
