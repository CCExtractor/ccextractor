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

#ifndef LIBEXPLAIN_BUFFER_UTIMENSAT_FLAGS_H
#define LIBEXPLAIN_BUFFER_UTIMENSAT_FLAGS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_utimensat_flags function may be used
  * to print a representation of a utimensat flags value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The utimensat flags value to be printed.
  */
void explain_buffer_utimensat_flags(explain_string_buffer_t *sb, int value);

/**
  * The explain_allbits_utimensat_flags function is used to get a bit
  * mask of the valid options.  This helps in an EINVAL explanation.
  */
int explain_allbits_utimensat_flags(void);

/**
  * The explain_parse_utimensat_flags function is used to parse a text
  * string to extract a suitable 'flags' argument for the utimensat
  * function.
  *
  * @param text
  *     The text string to be parsed.
  * @param result
  *     where to put the result
  * @returns
  *     0 on success, < 0 on failure
  */
int explain_parse_utimensat_flags(const char *text, int *result);

/**
  * The explain_parse_utimensat_flags function is used to
  *
  * @param text
  *    The text string to be parsed for a flags value.
  * @param caption
  *    Caption to add to start of error message, or NULL for none
  * @returns
  *    The value of the expression.  Does not return on error, but
  *    prints diagnostic and exits EXIT_FAILURE.
  * @note
  *    this function is <b>not</b> thread safe
  */
int explain_parse_utimensat_flags_or_die(const char *text, const char *caption);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_UTIMENSAT_FLAGS_H */
