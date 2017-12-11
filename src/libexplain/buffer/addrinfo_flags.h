/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ADDRINFO_FLAGS_H
#define LIBEXPLAIN_BUFFER_ADDRINFO_FLAGS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_addrinfo_flags function may be used to
  *
  * @param sb
  *    The string buffer to print into.
  * @param flgs
  *    the addrinfo.ai_flags value to be printed
  */
void explain_buffer_addrinfo_flags(explain_string_buffer_t *sb, int flgs);

/**
  * The explain_parse_addrinfo_flags_or_die function may be used to
  * parse a tet string into a addrinfo.ai_flags value.  On error, prints
  * diagnostic and exits EXIT_FAILURE.
  *
  * @param text
  *     The text to be parsed.
  * @param captn
  *     Additional text for fron of error message
  * @returns
  *     flags value
  */
int explain_parse_addrinfo_flags_or_die(const char *text, const char *captn);

#endif /* LIBEXPLAIN_BUFFER_ADDRINFO_FLAGS_H */
/* vim: set ts=8 sw=4 et : */
