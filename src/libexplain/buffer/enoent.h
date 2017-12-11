/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ENOENT_H
#define LIBEXPLAIN_BUFFER_ENOENT_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_enoent function may be used to print a
  * consistent explanation of ENOENT errors across system calls.
  *
  * @param sb
  *    The string buffer to print the explanation into
  * @param pathname
  *    The offending path
  * @param pathname_caption
  *    The name of the system call argument of the offending path
  * @param pathname_final_component
  *    the attributes required of the final path component
  */
void explain_buffer_enoent(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption,
    const struct explain_final_t *pathname_final_component);

/**
  * The explain_buffer_enoent function may be used to print a
  * consistent explanation of ENOENT errors across system calls.
  *
  * @param sb
  *    The string buffer to print the explanation into
  * @param oldpath
  *    The offending path
  * @param oldpath_caption
  *    The name of the system call argument of the offending path
  * @param oldpath_final_component
  *    the attributes required of the final path component
  * @param newpath
  *    The offending path
  * @param newpath_caption
  *    The name of the system call argument of the offenting path
  * @param newpath_final_component
  *    the attributes required of the final path component
  */
void explain_buffer_enoent2(explain_string_buffer_t *sb,
    const char *oldpath, const char *oldpath_caption,
    const struct explain_final_t *oldpath_final_component,
    const char *newpath, const char *newpath_caption,
    const struct explain_final_t *newpath_final_component);

#endif /* LIBEXPLAIN_BUFFER_ENOENT_H */
/* vim: set ts=8 sw=4 et : */
