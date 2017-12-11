/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_PRIO_WHICH_H
#define LIBEXPLAIN_BUFFER_PRIO_WHICH_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_prio_which function may be used
  * to print a representation of a getpriority which value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The getpriority which value to be printed.
  */
void explain_buffer_prio_which(explain_string_buffer_t *sb, int value);

/**
  * The explain_parse_prio_which_or_die function is used to parse the
  * given text op extract a getpriority which value.
  *
  * @param text
  *     The text to be parsed.  of this is a command line argument.
  * @param caption
  *     Additional information to make the error message more informatiove.
  * @returns
  *     getpriority which value
  *
  * @note
  *     if a pare error occurs, this function does not returns.  But
  *     rather exits EXIT_FAILUE.
  */
int explain_parse_prio_which_or_die(const char *text, const char *caption);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_PRIO_WHICH_H */
