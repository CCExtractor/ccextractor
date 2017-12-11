/*
 * libexplain - a library of system-call-specific strerror replacements
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

#ifndef LIBEXPLAIN_BUFFER_PTRACE_OPTIONS_H
#define LIBEXPLAIN_BUFFER_PTRACE_OPTIONS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_ptrace_options function may be used
  * to print a representation of a ptrace_options structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The ptrace(2) options value to be printed.
  */
void explain_buffer_ptrace_options(explain_string_buffer_t *sb, long value);

/**
  * The explain_parse_ptrace_options_or_die function is used to parse a
  * text string to extract a ptrace(2) options bit-map value.
  *
  * @param text
  *     The string to be parsed.
  * @param caption
  *     The additional text for the error message, describing the source
  *     of the text.
  * @returns
  *     The numeric value of the options.
  * @note
  *     This function does not return if there is an error, but rather
  *     prints an error message and exits.
  */
long explain_parse_ptrace_options_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_PTRACE_OPTIONS_H */
/* vim: set ts=8 sw=4 et : */
