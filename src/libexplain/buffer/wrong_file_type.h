/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_WRONG_FILE_TYPE_H
#define LIBEXPLAIN_BUFFER_WRONG_FILE_TYPE_H

#include <libexplain/string_buffer.h>

struct stat; /* forward */

/**
  * The explain_buffer_wrong_file_type function may be used to
  * explain that a file descriptor system call argument refers to a file
  * of the wrong file type.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The file descriptor that has the wrong file type.
  * @param caption
  *    The name of the system call argument that contains the file
  *    descriptor that has the wrong file type.
  * @param required_file_type
  *    The required file type, that the file descriptor does not match.
  */
void explain_buffer_wrong_file_type(explain_string_buffer_t *sb,
    int fildes, const char *caption, int required_file_type);

/**
  * The explain_buffer_wrong_file_type function may be used to
  * explain that a file descriptor system call argument refers to a file
  * of the wrong file type.
  *
  * @param sb
  *    The string buffer to print into.
  * @param st
  *    The file status describing the wrong file type.
  * @param caption
  *    The name of the system call argument that contains the file
  *    descriptor that has the wrong file type.
  * @param required_file_type
  *    The required file type, that the file descriptor does not match.
  */
void explain_buffer_wrong_file_type_st(explain_string_buffer_t *sb,
    const struct stat *st, const char *caption, int required_file_type);

#endif /* LIBEXPLAIN_BUFFER_WRONG_FILE_TYPE_H */
/* vim: set ts=8 sw=4 et : */
