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

#ifndef LIBEXPLAIN_BUFFER_V4L2_FMTDESC_H
#define LIBEXPLAIN_BUFFER_V4L2_FMTDESC_H

#include <libexplain/string_buffer.h>

struct v4l2_fmtdesc; /* forward */

/**
  * The explain_buffer_v4l2_fmtdesc function may be used
  * to print a representation of a v4l2_fmtdesc structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_fmtdesc structure to be printed.
  * @param extra
  *     whether or not to also print the returned value fields
  */
void explain_buffer_v4l2_fmtdesc(explain_string_buffer_t *sb,
    const struct v4l2_fmtdesc *data, int extra);

/**
  * The explain_v4l2_fmtdesc_get_nformats function may be used to
  * determine the number of vailable pixel format descriptions.
  *
  * @param fildes
  *     The associated file descriptor
  * @param type
  *     The type of pixel format of interest
  * @returns
  *     >0 the number of formats available,
  *     0 there are no formats available,
  *     <0 unable to determine number of available formats
  */
int explain_v4l2_fmtdesc_get_nformats(int fildes, int type);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_FMTDESC_H */
