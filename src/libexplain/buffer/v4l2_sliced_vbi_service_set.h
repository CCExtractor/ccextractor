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

#ifndef LIBEXPLAIN_BUFFER_V4L2_SLICED_VBI_SERVICE_SET_H
#define LIBEXPLAIN_BUFFER_V4L2_SLICED_VBI_SERVICE_SET_H

#include <libexplain/ac/stdint.h>
#include <libexplain/ac/stddef.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_v4l2_sliced_vbi_service_set function may be used
  * to print a representation of a v4l2_sliced_vbi service_set value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The v4l2_sliced_vbi service_set value to be printed.
  */
void explain_buffer_v4l2_sliced_vbi_service_set(explain_string_buffer_t *sb,
    int data);

void explain_buffer_v4l2_sliced_vbi_service_set_array(
    explain_string_buffer_t *sb, const uint16_t *data, size_t data_size);

void explain_buffer_v4l2_sliced_vbi_service_set_array2(
    explain_string_buffer_t *sb, const uint16_t *data, size_t dim1_size,
    size_t dim2_size);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_SLICED_VBI_SERVICE_SET_H */
