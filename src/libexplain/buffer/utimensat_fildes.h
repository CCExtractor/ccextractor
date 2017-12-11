/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_UTIMENSAT_FILDES_H
#define LIBEXPLAIN_BUFFER_UTIMENSAT_FILDES_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_utimensat_fildes function may be used
  * to print a representation of a utimensat fildes value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The utimensat fildes value to be printed.
  */
void explain_buffer_utimensat_fildes(explain_string_buffer_t *sb, int value);

/**
  * The explain_parse_utimensat_fildes_or_die function is used to parse
  * a text string to extract a file descriptor number, or possibly
  * AT_FDCWD referring to the current directory for *at functions.
  *
  * If the parse fails, it exits via the normal libexplain mechanism.
  *
  * @param text
  *     The text string to be parsed
  * @param caption
  *     additional context for error messages
  */
int explain_parse_utimensat_fildes_or_die(const char *text,
    const char *caption);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_UTIMENSAT_FILDES_H */
