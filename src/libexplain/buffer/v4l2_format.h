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

#ifndef LIBEXPLAIN_BUFFER_V4L2_FORMAT_H
#define LIBEXPLAIN_BUFFER_V4L2_FORMAT_H

#include <libexplain/string_buffer.h>

struct v4l2_format; /* forward */

/**
  * The explain_buffer_v4l2_format function may be used
  * to print a representation of a v4l2_format structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_format structure to be printed.
  * @param extra
  *     print the return value fields as well
  */
void explain_buffer_v4l2_format(explain_string_buffer_t *sb,
    const struct v4l2_format *data, int extra);

enum explain_v4l2_format_check_type_t
{
    explain_v4l2_format_check_type_no_idea,
    explain_v4l2_format_check_type_nosuch,
    explain_v4l2_format_check_type_notsup,
    explain_v4l2_format_check_type_ok
};

/**
  * The explain_v4l2_format_check_type function is used to check the type
  * member of a v4l2_format struct.
  *
  * @param fildes
  *     The associated file descriptor
  * @param data
  *     The control struct to have its id member checked.
  * @returns
  *     no_idea if the file descriptor is unhelpful or the pointer is efault,
  *     nosuch if there is no such type,
  *     notsup if the type exists, but is not supported by the device,
  *     ok if nothing wrong could be found.
  */
enum explain_v4l2_format_check_type_t explain_v4l2_format_check_type(int fildes,
    const struct v4l2_format *data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_FORMAT_H */
