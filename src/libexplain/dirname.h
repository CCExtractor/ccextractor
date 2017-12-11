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

#ifndef LIBEXPLAIN_DIRNAME_H
#define LIBEXPLAIN_DIRNAME_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_dirname function may be used to extract the directory
  * part of a path.
  *
  * @param dir
  *    The array in which to place the directory part.
  * @param path
  *    The path from which to extract the directory part.
  * @param dir_size
  *    The size in bytes of the destination dir array.
  */
void explain_dirname(char *dir, const char *path, size_t dir_size);

/**
  * The explain_basename function may be used to extract the final
  * component of a path.
  *
  * @param filnam
  *     The array to receive the filename
  * @param path
  *     The path from which to extract the final component.
  * @param filnam_size
  *     The size of the array to receive the file name
  */
void explain_basename(char *filnam, const char *path, size_t filnam_size);

#endif /* LIBEXPLAIN_DIRNAME_H */
/* vim: set ts=8 sw=4 et : */
