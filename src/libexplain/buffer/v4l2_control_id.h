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

#ifndef LIBEXPLAIN_BUFFER_V4L2_CONTROL_ID_H
#define LIBEXPLAIN_BUFFER_V4L2_CONTROL_ID_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_v4l2_control_id function may be used
  * to print a representation of a v4l2_control_id structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2 control id value to be printed.
  */
void explain_buffer_v4l2_control_id(explain_string_buffer_t *sb, int data);

enum explain_v4l2_control_id_check_t
{
    explain_v4l2_control_id_check_no_idea,
    explain_v4l2_control_id_check_nosuch,
    explain_v4l2_control_id_check_notsup,
    explain_v4l2_control_id_check_ok
};

/**
  * The explain_v4l2_control_id_check function is used to check a control id.
  *
  * @param fildes
  *     The associated file descriptor
  * @param data
  *     The control id value to be checked.
  * @returns
  *     no_idea if the file descriptor is unhelpful or the pointer is efault,
  *     nosuch if there is no such control id,
  *     notsup if the control id exists, but is not supported by the device,
  *     ok if nothing wrong could be found.
  */
enum explain_v4l2_control_id_check_t explain_v4l2_control_id_check(int fildes,
    int data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_CONTROL_ID_H */
