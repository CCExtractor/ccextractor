/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_OPEN_FLAGS_H
#define LIBEXPLAIN_OPEN_FLAGS_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_open_flags function may be used to decode the flags
  * argument to the open(2) system call into a humnan readable string,
  * which is one representation of how it would be written by a coder.
  *
  * @param flags
  *    the flags argument passed to the open(2) system call, second argument
  * @returns
  *    pointer to string constructed in shared buffer
  * @note
  *    not thread safe because of shared buffer
  */
const char *explain_open_flags(int flags);

/**
  * The explain_message_open_flags function may be used to decode the
  * flags argument to the open(2) system call into a humnan readable
  * string, which is one representation of how it would be written by a
  * coder.
  *
  * @param message
  *     where to put the constructed text.  This makes it
  *     thread safe, given a suitabley safe buffer.
  * @param message_size
  *     the size of the buffer receiving the message
  * @param flags
  *    the flags argument passed to the open(2) system call, second argument
  */
void explain_message_open_flags(char *message, size_t message_size,
    int flags);

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_open_flags function may be used to decode the
  * flags argument to the open(2) system call into a humnan readable
  * string, which is one representation of how it would be written by a
  * coder.
  *
  * @param sb
  *     where to put the constructed text.  This makes it
  *     thread safe, given a suitabley safe buffer.
  * @param flags
  *    the flags argument passed to the open(2) system call, second argument
  */
void explain_buffer_open_flags(struct explain_string_buffer_t *sb,
    int flags);

/**
  * The explain_pare_open_flags_or_die function may be used to parse
  * a strings containing a symbolic representation of open() flags,
  * turning it into a open flags value.
  *
  * @param text
  *     The text to be parsed to extract an open flags value.
  * @param caption
  *     additional text to add to start of error message
  * @returns
  *     open flags value
  * @note
  *     If there is a parse error, a fatal error message is printed,
  *     and exit(EXIT_FAILURE) is called.  If there is an error, this
  *     function will not return.
  */
int explain_parse_open_flags_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_OPEN_FLAGS_H */
/* vim: set ts=8 sw=4 et : */
