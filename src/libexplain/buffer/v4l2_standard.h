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

#ifndef LIBEXPLAIN_BUFFER_V4L2_STANDARD_H
#define LIBEXPLAIN_BUFFER_V4L2_STANDARD_H

#include <libexplain/string_buffer.h>

struct v4l2_standard; /* forward */

/**
  * The explain_buffer_v4l2_standard function may be used
  * to print a representation of a v4l2_standard structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_standard structure to be printed.
  * @param extra
  *     whether or not to also print the returned value fields
  */
void explain_buffer_v4l2_standard(explain_string_buffer_t *sb,
    const struct v4l2_standard *data, int extra);

/**
  * The explain_v4l2_get_nstandards function is used to obtain the
  * number of output video standards supported by the device.
  *
  * @param fildes
  *     The file descriptor of the open device.
  * @returns
  *     >0 for the number of standards,
  *     =0 when zero standards are supported,
  *     <0 for unable to determine number of supported standards
  */
int explain_v4l2_get_nstandards(int fildes);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_STANDARD_H */
