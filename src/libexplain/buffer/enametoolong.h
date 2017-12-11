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

#ifndef LIBEXPLAIN_BUFFER_ENAMETOOLONG_H
#define LIBEXPLAIN_BUFFER_ENAMETOOLONG_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_enametoolong function may be used to explan an
  * ENAMETOOLONG error, in a manner consisten across all system calls.
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The pathname to be explained
  * @param pathname_caption
  *    The argument name of the pathname to be explained
  * @param pathname_final_component
  *    The desired properties of the final component of the pathname
  */
void explain_buffer_enametoolong(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption,
    const struct explain_final_t *pathname_final_component);

/**
  * The explain_buffer_enametoolong2 function may be used to explan
  * an ENAMETOOLONG error, in a manner consistent across all system
  * calls that require two path arguments.
  *
  * @param sb
  *    The string buffer to print into.
  * @param oldpath
  *    The first pathname to be explained
  * @param oldpath_caption
  *    The argument name of the first pathname to be explained
  * @param oldpath_final_component
  *    The desired properties of the final component of the oldpath
  * @param newpath
  *    The second pathname to be explained
  * @param newpath_caption
  *    The argument name of the second pathname to be explained
  * @param newpath_final_component
  *    The desired properties of the final component of the newpath
  */
void explain_buffer_enametoolong2(explain_string_buffer_t *sb,
    const char *oldpath, const char *oldpath_caption,
    const struct explain_final_t *oldpath_final_component,
    const char *newpath, const char *newpath_caption,
    const struct explain_final_t *newpath_final_component);

/**
  * The explain_buffer_enametoolong_gethostname function is sued to
  * print an explaination for an ENAMETOOLONG error reported by the
  * gethostname, getdomainname (etc) functions.
  *
  * @param sb
  *     The string buffer to print into.
  * @param actual_length
  *     The actual length of the host/domain name, this will be larger
  *     than the value passed.
  * @param caption
  *     The name of the offending system call argument
  */
void explain_buffer_enametoolong_gethostname(explain_string_buffer_t *sb,
    int actual_length, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_ENAMETOOLONG_H */
/* vim: set ts=8 sw=4 et : */
