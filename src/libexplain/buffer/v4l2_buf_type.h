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

#ifndef LIBEXPLAIN_BUFFER_V4L2_BUF_TYPE_H
#define LIBEXPLAIN_BUFFER_V4L2_BUF_TYPE_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_v4l2_buf_type function may be used
  * to print a representation of a v4l2_buf_type structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The v4l2_buf_type value to be printed.
  */
void explain_buffer_v4l2_buf_type(explain_string_buffer_t *sb, int value);

/**
  * The explain_buffer_v4l2_buf_type function may be used
  * to print a representation of a v4l2_buf_type structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The v4l2_buf_type value to be printed.
  */
void explain_buffer_v4l2_buf_type_ptr(explain_string_buffer_t *sb,
    const int *value);

enum explain_v4l2_buf_type_check_t
{
    explain_v4l2_buf_type_check_no_idea, /* unable to query device */
    explain_v4l2_buf_type_check_notsup,  /* valid buf type, no support by dev */
    explain_v4l2_buf_type_check_nosuch,  /* not a valid buf type */
    explain_v4l2_buf_type_check_ok       /* supported by the device */
};

/**
  * The explain_v4l2_buf_type_check function is used to check the
  * validity of a v4l2 buffer type.
  *
  * @param fildes
  *     The associated file descriptor.
  * @param data
  *     The alleged v4l2 buffer type to be checked.
  * @returns
  *     no_idea if the file descriptor is unhelpful, no if the data is
  *     not a valid buffer type, and yes of it is valif and supported by
  *     the device (fildes).
  */
enum explain_v4l2_buf_type_check_t explain_v4l2_buf_type_check(int fildes,
    int data);

/**
  * The explain_v4l2_buf_type_ptr_check function is used to check the
  * validity of a v4l2 buffer type.
  *
  * @param fildes
  *     The associated file descriptor.
  * @param data
  *     The alleged v4l2 buffer type to be checked.
  * @returns
  *     no_idea if the file descriptor is unhelpful or the pointer is
  *     efault, no if the data is not a valid buffer type, and yes of it
  *     is valif and supported by the device (fildes).
  */
enum explain_v4l2_buf_type_check_t explain_v4l2_buf_type_ptr_check(int fildes,
    const int *data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_V4L2_BUF_TYPE_H */
