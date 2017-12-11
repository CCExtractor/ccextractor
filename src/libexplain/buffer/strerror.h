/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_STRERROR_H
#define LIBEXPLAIN_BUFFER_STRERROR_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_strerror function may be used to insert
  * strerror text into the given buffer, along with its numkber and
  * name.
  *
  * @param sb
  *    The string buffer to write the results to
  * @param errnum
  *    The errno value to be described.
  */
void explain_buffer_strerror(struct explain_string_buffer_t *sb,
    int errnum);

#endif /* LIBEXPLAIN_BUFFER_STRERROR_H */
/* vim: set ts=8 sw=4 et : */
