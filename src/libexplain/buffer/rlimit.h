/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_RLIMIT_H
#define LIBEXPLAIN_BUFFER_RLIMIT_H

#include <libexplain/ac/sys/resource.h>

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_rlimit function may be used to insert a human
  * readable representation of an rlimit struct into the given buffer.
  *
  * @param sb
  *    The string buffer to write into
  * @param value
  *    Pointer to the rlimit struct to be printed
  */
void explain_buffer_rlimit(struct explain_string_buffer_t *sb,
    const struct rlimit *value);

/**
  * The explain_buffer_rlim_t function may be used to insert a human
  * readable representation of an rlim_t struct into the given buffer.
  *
  * @param sb
  *    The string buffer to write into
  * @param value
  *    Pointer to the rlim_t value to be printed
  */
void explain_buffer_rlim_t(struct explain_string_buffer_t *sb, rlim_t value);

#endif /* LIBEXPLAIN_BUFFER_RLIMIT_H */
/* vim: set ts=8 sw=4 et : */
