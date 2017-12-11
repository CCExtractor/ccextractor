/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_SETVBUF_MODE_H
#define LIBEXPLAIN_BUFFER_SETVBUF_MODE_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_setvbuf_mode function may be used
  * to print a representation of a setvbuf mode value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The setvbuf mode value to be printed.
  */
void explain_buffer_setvbuf_mode(explain_string_buffer_t *sb, int value);

/**
  * The explain_parse_setvbuf_mode_or_die function is used to
  * parse a string into a setvbuf(3) mode argument.
  *
  * @param text
  *     The text to be parsed.
  * @param caption
  *     Additional information to add to the error message.
  */
int explain_parse_setvbuf_mode_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_SETVBUF_MODE_H */
/* vim: set ts=8 sw=4 et : */
