/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_ENOTDIR_H
#define LIBEXPLAIN_BUFFER_ENOTDIR_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_enotdir function may be used to
  *
  * @param sb
  *     The string buffer to print into
  * @param pathname
  *     The offending path
  * @param pathname_caption
  *     The name of the function argument of the offending path
  * @param pathname_final_component
  *     The required properties of the final component of the offending path
  */
void explain_buffer_enotdir(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption,
    const struct explain_final_t *pathname_final_component);

/**
  * The explain_buffer_enotdir function may be used to
  *
  * @param sb
  *     The string buffer to print into
  * @param oldpath
  *     The offending path
  * @param oldpath_caption
  *     The name of the function argument of the offending path
  * @param oldpath_final_component
  *     The required properties of the final component of the offending path
  * @param newpath
  *     The offending path
  * @param newpath_caption
  *     The name of the function argument of the offending path
  * @param newpath_final_component
  *     The required properties of the final component of the offending path
  */
void explain_buffer_enotdir2(explain_string_buffer_t *sb,
    const char *oldpath, const char *oldpath_caption,
    const struct explain_final_t *oldpath_final_component,
    const char *newpath, const char *newpath_caption,
    const struct explain_final_t *newpath_final_component);

/**
  * The explain_buffer_enotdir_fd function may be used to
  *
  * @param sb
  *     The string buffer to print into
  * @param fildes
  *     The offending file descriptor
  * @param fildes_caption
  *     The name of the function argument of the offending file descriptor
  */
void explain_buffer_enotdir_fd(explain_string_buffer_t *sb,
    int fildes, const char *fildes_caption);

/**
  * The explain_fildes_is_a_directory function is used to determine
  * whether or not a file descriptor refers to a directory.
  *
  * @param fildes
  *     The file descriptor to be tested.
  * @returns
  *     true (nonzero0 if it is a dir;
  *     false (zero) is it is not a dir.
  */
int explain_fildes_is_a_directory(int fildes);

#endif /* LIBEXPLAIN_BUFFER_ENOTDIR_H */
/* vim: set ts=8 sw=4 et : */
