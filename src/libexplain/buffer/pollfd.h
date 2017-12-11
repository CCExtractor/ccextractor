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

#ifndef LIBEXPLAIN_BUFFER_POLLFD_H
#define LIBEXPLAIN_BUFFER_POLLFD_H

#include <libexplain/string_buffer.h>

struct pollfd; /* forward */

/**
  * The explain_buffer_pollfd function may be used
  * to print a representation of a pollfd structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The pollfd structure to be printed.
  * @param include_revents
  *     true (non-zero) of the revents field should be printed, false
  *     (zero) if the revents field should not be printed.
  */
void explain_buffer_pollfd(explain_string_buffer_t *sb,
    const struct pollfd *data, int include_revents);

/**
  * The explain_buffer_pollfd_array function may be used
  * to print a representation of an array of pollfd structures.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     a pointer to the base of the array of pollfd structures to be printed.
  * @param data_size
  *     The number of pollfd structures to be printed.
  * @param include_revents
  *     true (non-zero) of the revents field should be printed, false
  *     (zero) if the revents field should not be printed.
  */
void explain_buffer_pollfd_array(explain_string_buffer_t *sb,
    const struct pollfd *data, int data_size, int include_revents);

#endif /* LIBEXPLAIN_BUFFER_POLLFD_H */
/* vim: set ts=8 sw=4 et : */
