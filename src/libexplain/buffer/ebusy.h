/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013, 2014 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_EBUSY_H
#define LIBEXPLAIN_BUFFER_EBUSY_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_ebusy_fildes function may be used
  * to print an explanation of an EBUSY error.
  *
  * @param sb
  *     The string buffer to print into.
  * @param fildes
  *     The open file descriptor of the file (device) with the problem.
  * @param fildes_caption
  *     The name of the offending syscall argument
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_ebusy_fildes(explain_string_buffer_t *sb, int fildes,
    const char *fildes_caption, const char *syscall_name);

/**
  * The explain_buffer_ebusy_path function may be used
  * to print an explanation of an EBUSY error.
  *
  * @param sb
  *     The string buffer to print into.
  * @param path
  *     The path of the file (device) with the problem.
  * @param path_caption
  *     The name of the offending syscall argument
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_ebusy_path(explain_string_buffer_t *sb, const char *path,
    const char *path_caption, const char *syscall_name);

#endif /* LIBEXPLAIN_BUFFER_EBUSY_H */
/* vim: set ts=8 sw=4 et : */
