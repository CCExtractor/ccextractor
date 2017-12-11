/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_SHMCTL_COMMAND_H
#define LIBEXPLAIN_BUFFER_SHMCTL_COMMAND_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_shmctl_command function may be used
  * to print a representation of a shmctl_command structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The shmctl command valie to be printed.
  */
void explain_buffer_shmctl_command(explain_string_buffer_t *sb, int value);

/**
  * The explain_parse_shmctl_command_or_die function is used to parse a
  * text string, to extract a shmctl(2) command argument value.
  *
  * @param text
  *     The text to be parsed.
  * @param caption
  *     an additonal descripotion, for error messages
  * @returns
  *     the chmctl(2) command.  On error, it does not return.
  */
int explain_parse_shmctl_command_or_die(const char *text, const char *caption);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_SHMCTL_COMMAND_H */
