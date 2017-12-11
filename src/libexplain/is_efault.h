/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_PATH_IS_EFAULT_H
#define LIBEXPLAIN_PATH_IS_EFAULT_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_is_efault_path function may be used to determine whether
  * or not a path pointer is a valid pointer.  Typically this is used
  * when handling ambiguous EFAULT situations.
  *
  * @param path
  *    The path to be checked.
  * @returns
  *    int; 1 if the path is bad (points outside the process address
  *    space), or 0 if the path is OK.
  */
int explain_is_efault_path(const char *path);

/**
  * The explain_is_efault_pointer function may be used to determine
  * whether or not a pointer is valid.  Typically this is used when
  * handling ambiguous EFAULT situations.
  *
  * @param data
  *    Pointer to the base address of the memory to be checked.
  * @param data_size
  *    The size in bytes of the memory to be checked
  *    (i.e. you can use sizeof(*data) in most situations).
  * @returns
  *    int; 1 if the pointer is bad (points outside the process address
  *    space), or 0 if the pointer is OK.
  */
int explain_is_efault_pointer(const void *data, size_t data_size);

/**
  * The explain_is_efault_string function may be used to determine
  * whether or not a C string is valid.  Typically this is used when
  * handling ambiguous EFAULT situations.
  *
  * @param data
  *    Pointer to the base address of the memory to be checked.
  * @returns
  *    int; 1 if the C string is bad (points outside the process address
  *    space), or 0 if the C string is OK.
  */
int explain_is_efault_string(const char *data);

#endif /* LIBEXPLAIN_PATH_IS_EFAULT_H */
/* vim: set ts=8 sw=4 et : */
