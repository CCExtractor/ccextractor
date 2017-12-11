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

#ifndef LIBEXPLAIN_BUFFER_ACCEPT4_FLAGS_H
#define LIBEXPLAIN_BUFFER_ACCEPT4_FLAGS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_accept4_flags function may be used
  * to print a representation of the accept4() flags argument.
  *
  * @param sb
  *     The string buffer to print into.
  * @param flags
  *     The accept4() flags value to be printed.
  */
void explain_buffer_accept4_flags(explain_string_buffer_t *sb, int flags);

/**
  * The explain_accept4_flags_parse_or_die function is used to convert a
  * text string into an accept4() flags value.
  *
  * @param text
  *     The text to be converted.
  * @param caption
  *     An extra caption for error messages.
  * @returns
  *     the parsed value
  */
int explain_accept4_flags_parse_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_ACCEPT4_FLAGS_H */
/* vim: set ts=8 sw=4 et : */
