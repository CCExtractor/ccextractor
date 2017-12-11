/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_TIMESPEC_H
#define LIBEXPLAIN_BUFFER_TIMESPEC_H

#include <libexplain/string_buffer.h>

struct timespec; /* forward */

/**
  * The explain_buffer_timespec function may be used
  * to print a representation of a timespec structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The timespec structure to be printed.
  */
void explain_buffer_timespec(explain_string_buffer_t *sb,
    const struct timespec *data);

/**
  * The explain_buffer_timespec_array function may be used
  * to print a representation of a timespec structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The timespec structure to be printed.
  * @param data_size
  *     The number of elements in the array.
  */
void explain_buffer_timespec_array(explain_string_buffer_t *sb,
    const struct timespec *data, unsigned data_size);

/**
  * The explain_parse_timespec_or_die function is used to parse some
  * text, to extract a timespec value.  It could be a floating point
  * number of seconds, it coule be one of the UTIME_* values, it could
  * be free text as permitted by explain_parse_time_t
  *
  * On error, ithis function does not return, but errors out using the
  * usual libexplain error handling.
  *
  * @param text
  *     The text string to be parsed.
  * @param caption
  *     additionl context for fatal error message, if necessary
  * @param result
  *     where to put the answer on success
  */
void explain_parse_timespec_or_die(const char *text, const char *caption,
    struct timespec *result);

#endif /* LIBEXPLAIN_BUFFER_TIMESPEC_H */
/* vim: set ts=8 sw=4 et : */
