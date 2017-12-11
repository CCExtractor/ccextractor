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

#ifndef LIBEXPLAIN_BUFFER_SIGNALFD_FLAGS_H
#define LIBEXPLAIN_BUFFER_SIGNALFD_FLAGS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_signalfd_flags function may be used
  * to print a representation of a signalfd flags argument.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The signalfd flags value to be printed.
  */
void explain_buffer_signalfd_flags(explain_string_buffer_t *sb, int data);

/**
  * The explain_parse_signalfd_flags_or_die function is used to parse a
  * text representation of a signalfd flags argument.
  *
  * @param text
  *     The text to be parsed.
  * @param caption
  *     additional information for error messages
  * @returns
  *     the flags value.  Do not return on error, but instead prints a
  *     fatal error message and exits.
  */
int explain_parse_signalfd_flags_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_SIGNALFD_FLAGS_H */
/* vim: set ts=8 sw=4 et : */
