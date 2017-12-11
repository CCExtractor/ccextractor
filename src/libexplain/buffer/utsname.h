/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPALIN_BUFFER_UTSNAME_H
#define LIBEXPALIN_BUFFER_UTSNAME_H

#include <libexplain/string_buffer.h>

struct utsname; /* forward */

/**
  * The explain_buffer_utsname function may be used to print a
  * representation of a striuvt utsname value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The ustaname value to be printed.
  */
void explain_buffer_utsname(explain_string_buffer_t *sb,
    const struct utsname *data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPALIN_BUFFER_UTSNAME_H */
