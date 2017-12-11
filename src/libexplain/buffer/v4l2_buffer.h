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

#ifndef LIBEXPLAIN_BUFFER_V4L2_BUFFER_H
#define LIBEXPLAIN_BUFFER_V4L2_BUFFER_H

#include <libexplain/string_buffer.h>

struct v4l2_buffer; /* forward */

/**
  * The explain_buffer_v4l2_buffer function may be used
  * to print a representation of a v4l2_buffer structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_buffer structure to be printed.
  * @param extra
  *     whether or not to print the returned data fields as well
  */
void explain_buffer_v4l2_buffer(explain_string_buffer_t *sb,
    const struct v4l2_buffer *data, int extra);

/**
  * The explain_v4l2_buffer_get_nframes function is used to
  * determine how many frames have been allocated.  There is no ioctl to
  * ask directly how many frames have been allocated, so we will have to
  * binary chop.
  *
  * @param fildes
  *     The file descriptor of the video device.
  * @param type
  *     The memory type of the buffer.
  * @returns
  *     the number of buffers, or 0 if no buffers have been allocated yet,
  *     or -1 if the ioctl returned something strange.
  */
int explain_v4l2_buffer_get_nframes(int fildes, int type);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_BUFFER_H */
