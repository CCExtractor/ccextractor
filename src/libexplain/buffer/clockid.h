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

#ifndef LIBEXPLAIN_BUFFER_CLOCKID_H
#define LIBEXPLAIN_BUFFER_CLOCKID_H

#include <libexplain/string_buffer.h>

struct clockid; /* forward */

/**
  * The explain_buffer_clockid function may be used
  * to print a representation of a clockid value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The clockid value to be printed.
  */
void explain_buffer_clockid(explain_string_buffer_t *sb, int value);

/**
  * The explain_parse_clockid_or_die function is used to parse a text
  * string ito a clockid value.
  *
  * @param text
  *     The text string to be parse for a timerid value.
  * @param caption
  *     additional text for error messages
  * @returns
  *     the clockid value.  Does not return on error.
  */
int explain_parse_clockid_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_CLOCKID_H */
/* vim: set ts=8 sw=4 et : */
