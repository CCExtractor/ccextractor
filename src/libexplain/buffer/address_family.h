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

#ifndef LIBEXPLAIN_BUFFER_ADDRESS_FAMILY_H
#define LIBEXPLAIN_BUFFER_ADDRESS_FAMILY_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_address_family function may be used to print a
  * symbolic value of a socket address family value to the given string
  * buffer.
  *
  * @param sb
  *     The string buffer to print into.
  * @param domain
  *     The socket address family value to interpret.
  */
void explain_buffer_address_family(explain_string_buffer_t *sb,
    int domain);

/**
  * The explain_parse_address_family_or_die function is used to parse
  * a string into a socket address family value.  On error, whill prind
  * diagnostic and exit EXIT_FAILURE.
  *
  * @param text
  *     The string to be parsed.
  * @param captn
  *     additional text to add to start of error message
  * @returns
  *     the socket address family
  */
int explain_parse_address_family_or_die(const char *text, const char *captn);

#endif /* LIBEXPLAIN_BUFFER_ADDRESS_FAMILY_H */
/* vim: set ts=8 sw=4 et : */
