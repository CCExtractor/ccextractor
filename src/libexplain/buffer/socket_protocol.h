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

#ifndef LIBEXPLAIN_BUFFER_SOCKET_PROTOCOL_H
#define LIBEXPLAIN_BUFFER_SOCKET_PROTOCOL_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_socket_protocol function may be used to print a
  * symbolic value of a socket protocol value to the given string buffer.
  *
  * @param sb
  *     The string buffer to print into.
  * @param protocol
  *     The socket protocol value to interpret.
  */
void explain_buffer_socket_protocol(struct explain_string_buffer_t *sb,
    int protocol);

/**
  * The explain_parse_socket_protocol function is used to parse a
  * string into a socket protocol value.  On error, prints a diagnostic
  * and exits EXIT_FAILURE.
  *
  * @param text
  *     The string to be parsed.
  * @param cptn
  *     additional text to add to start of error message
  * @returns
  *     the socket value, or -1 on error
  */
int explain_parse_socket_protocol_or_die(const char *text, const char *cptn);

#endif /* LIBEXPLAIN_BUFFER_SOCKET_PROTOCOL_H */
/* vim: set ts=8 sw=4 et : */
