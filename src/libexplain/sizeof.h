/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_SIZEOF_H
#define LIBEXPLAIN_SIZEOF_H

/**
  * The SIZEOF macro is used to calculate the number of
  * elements in an array.  This is usedful in cases when the C built-in
  * sizeof operator would return the size of the array in bytes, but the
  * array elements are larger in size than a single byte.
  *
  * @param a
  *     The array of interest
  */
#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

/**
  * The ENDOF macro is used to calculate the address of
  * the array element just of the end of an array.  This is useful in
  * writing for loops to traverse arrays usuing an elelent pointer.
  *
  * @param a
  *     The array of interest
  */
#define ENDOF(a) ((a) + SIZEOF(a))

#endif /* LIBEXPLAIN_SIZEOF_H */
/* vim: set ts=8 sw=4 et : */
