/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_WAITPID_OPTIONS_H
#define LIBEXPLAIN_BUFFER_WAITPID_OPTIONS_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_waitpid_options function may be used to
  * decode and print waitpid(2) options.
  *
  * @param sb
  *     The string buffer to print into
  * @param options
  *     The options to be decoded
  */
void explain_buffer_waitpid_options(struct explain_string_buffer_t *sb,
    int options);

/**
  * The explain_parse_waitpid_options function is used to parse text
  * containing waitpid options into a numeric value.
  *
  * @param text
  *     The string to be parsed
  * @param caption
  *     additional text to be added to theh start of error messages
  * @returns
  *     the flags value
  */
int explain_parse_waitpid_options_or_die(const char *text,
    const char *caption);

#endif /* LIBEXPLAIN_BUFFER_WAITPID_OPTIONS_H */
/* vim: set ts=8 sw=4 et : */
