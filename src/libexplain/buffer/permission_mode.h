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

#ifndef LIBEXPLAIN_BUFFER_PERMISSION_MODE_H
#define LIBEXPLAIN_BUFFER_PERMISSION_MODE_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_permission_mode function may be used to decode
  * a set of permission mode bits into human readable text, something
  * like the programmer would have written.
  *
  * @param sb
  *    The string buffer to write the mode into.
  *    This makes the function thread safe if the buffer is suitable.
  * @param mode
  *    the permission mode bits to be decoded
  */
void explain_buffer_permission_mode(explain_string_buffer_t *sb, mode_t mode);

/**
  * The explain_permission_mode_parse_or_die function may be used to
  * parse a strings containing a symbolic representation of a permission
  * mode, turning it into a permission mode value.
  *
  * @param text
  *     The text to be parsed to extract a permission mode value.
  * @param caption
  *     additional text to add to the start of the error message
  * @returns
  *     permission mode value
  * @note
  *     If there is a parse error, a fatal error message is printed,
  *     and exit(EXIT_FAILURE) is called.  If there is an error, this
  *     function will not return.
  */
mode_t explain_permission_mode_parse_or_die(const char *text,
    const char *caption);

#endif /* LIBEXPLAIN_BUFFER_PERMISSION_MODE_H */
/* vim: set ts=8 sw=4 et : */
