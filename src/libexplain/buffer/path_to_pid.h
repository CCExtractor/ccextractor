/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_PATH_TO_PID_H
#define LIBEXPLAIN_BUFFER_PATH_TO_PID_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_path_to_pid function may be used to inject the
  * PIDs of processes that have the given file open.
  *
  * @param sb
  *    The string buffer to write the PIDs into
  * @param path
  *    The path of the file of interest
  * @returns
  *    number of uses
  */
int explain_buffer_path_to_pid(struct explain_string_buffer_t *sb,
    const char *path);

/**
  * The explain_buffer_fildes_to_pid function may be used to inject the
  * PIDs of processes that have the given file open.
  *
  * @param sb
  *    The string buffer to write the PIDs into
  * @param fildes
  *    open file descrptor of file of interest
  * @returns
  *    number of uses
  */
int explain_buffer_fildes_to_pid(struct explain_string_buffer_t *sb,
    int fildes);

/**
  * The explain_buffer_path_users function may be used to inject the
  * PIDs of processes that have the given file open.  The caption is
  * inserted, too.
  *
  * @param sb
  *    The string buffer to write the PIDs into
  * @param path
  *    The path of the file of interest
  * @param caption
  *    The name of the argument being checked, "oldpath", "newpath",
  *    "pathname" or similar.
  * @returns
  *    number of uses
  */
int explain_buffer_path_users(struct explain_string_buffer_t *sb,
    const char *path, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_PATH_TO_PID_H */
/* vim: set ts=8 sw=4 et : */
