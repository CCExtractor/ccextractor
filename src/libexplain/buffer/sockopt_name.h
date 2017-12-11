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

#ifndef LIBEXPLAIN_BUFFER_SOCKOPT_NAME_H
#define LIBEXPLAIN_BUFFER_SOCKOPT_NAME_H

#include <libexplain/string_buffer.h>

struct sockopt_name; /* forward */

/**
  * The explain_buffer_sockopt_name function may be used to
  * print a representation of a sockopt name.
  *
  * @param sb
  *     The string buffer to print into.
  * @param level
  *     The sockopt level value.  This is important because the names
  *     are dependent on the level.
  * @param name
  *     The sockopt_name value to be printed.
  */
void explain_buffer_sockopt_name(explain_string_buffer_t *sb, int level,
    int name);

/**
  * The explain_parse_sockopt_name_or_die function is used to parse a
  * text string into a sockopt name value.  It parses all levels.
  *
  * @param text
  *     The text string to be parsed.
  * @param caption
  *     additional information for error messages.
  * @returns
  *     The sockopt name value.  On error it prints a fatal error
  *     message and exits.
  */
int explain_parse_sockopt_name_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_SOCKOPT_NAME_H */
/* vim: set ts=8 sw=4 et : */
