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

#ifndef LIBEXPLAIN_BUFFER_V4L2_QUERYCTRL_H
#define LIBEXPLAIN_BUFFER_V4L2_QUERYCTRL_H

#include <libexplain/string_buffer.h>

struct v4l2_queryctrl; /* forward */

/**
  * The explain_buffer_v4l2_queryctrl function may be used
  * to print a representation of a v4l2_queryctrl structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_queryctrl structure to be printed.
  * @param extra
  *     print the return values if true.
  * @param fildes
  *     Thes associated file descriptor (for additional details, if necessary)
  */
void explain_buffer_v4l2_queryctrl(explain_string_buffer_t *sb,
    const struct v4l2_queryctrl *data, int extra, int fildes);

enum explain_v4l2_queryctrl_check_id_t
{
    explain_v4l2_queryctrl_check_id_no_idea,
    explain_v4l2_queryctrl_check_id_nosuch,
    explain_v4l2_queryctrl_check_id_notsup,
    explain_v4l2_queryctrl_check_id_ok
};

/**
  * The explain_v4l2_queryctrl_check_id function is used to check the id
  * member of a v4l2_queryctrl struct.
  *
  * @param fildes
  *     The associated file descriptor
  * @param data
  *     The control struct to have its id member checked.
  * @returns
  *     no_idea if the file descriptor is unhelpful or the pointer is efault,
  *     nosuch if there is no such control id,
  *     notsup if the control id exists, but is not supported by the device,
  *     ok if nothing wrong could be found.
  */
enum explain_v4l2_queryctrl_check_id_t explain_v4l2_queryctrl_check_id(
    int fildes, const struct v4l2_queryctrl *data);

#endif /* LIBEXPLAIN_BUFFER_V4L2_QUERYCTRL_H */
/* vim: set ts=8 sw=4 et : */
