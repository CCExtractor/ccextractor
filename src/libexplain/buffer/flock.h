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

#ifndef LIBEXPLAIN_BUFFER_FLOCK_H
#define LIBEXPLAIN_BUFFER_FLOCK_H

struct explain_string_buffer_t; /* forward */
struct flock; /* forward */
struct flock64; /* forward */

/**
  * The explain_buffer_flock function may be used to form a human
  * readable representation of an flock structure.
  *
  * @param sb
  *    The string buffer in which to write the representation of the
  *    flock structure.
  * @param flp
  *    pointer to the flock structure to be decoded
  */
void explain_buffer_flock(struct explain_string_buffer_t *sb,
    const struct flock *flp);

/**
  * The explain_buffer_flock64 function may be used to form a human
  * readable representation of an flock64 structure.
  *
  * @param sb
  *    The string buffer in which to write the representation of the
  *    flock structure.
  * @param flp
  *    pointer to the flock64 structure to be decoded
  */
void explain_buffer_flock64(struct explain_string_buffer_t *sb,
    const struct flock64 *flp);

#endif /* LIBEXPLAIN_BUFFER_FLOCK_H */
/* vim: set ts=8 sw=4 et : */
