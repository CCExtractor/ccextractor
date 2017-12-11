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

#ifndef LIBEXPLAIN_PERMISSION_MODE_H
#define LIBEXPLAIN_PERMISSION_MODE_H

#include <libexplain/ac/stddef.h>

struct explain_string_buffer_t; /* forward */

/**
  * The explain_message_permission_mode function may be used to
  * decode a set of permission mode bits into human readable text,
  * something like the programmer would have written.
  *
  * @param message
  *    The character array to write the mode into.
  *    This makes the function thread safe if the array is suitable.
  * @param message_size
  *    The size in bytes of the character array to write the mode into.
  * @param mode
  *    the permission mode bits to be decoded
  */
void explain_message_permission_mode(char *message, size_t message_size,
    int mode);

/**
  * The explain_permission_mode function may be used to decode a set
  * of permission mode bits into human readable text, something like the
  * programmer would have written.
  *
  * @param mode
  *    the permission mode bits to be decoded
  * @return
  *     pointer to string in shared buffer, it will be overwritten by
  *     subsequent calls to this function.
  * @note
  *    this function is not thread safe.
  */
const char *explain_permission_mode(int mode);

#endif /* LIBEXPLAIN_PERMISSION_MODE_H */
/* vim: set ts=8 sw=4 et : */
