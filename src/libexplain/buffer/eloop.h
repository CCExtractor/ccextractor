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

#ifndef LIBEXPLAIN_BUFFER_ELOOP_H
#define LIBEXPLAIN_BUFFER_ELOOP_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_eloop function may be used to provide a common
  * explanation for ELOOP for all system calls.
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The pathname being checked.
  * @param pathname_caption
  *    The argument name of the pathname being checked.
  * @param pathname_final_component
  *    The desired properties of the last component of pathname
  */
void explain_buffer_eloop(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption,
    const struct explain_final_t *pathname_final_component);

/**
  * The explain_buffer_eloop2 function may be used to provide a common
  * explanation for ELOOP for all system calls that take two path names.
  *
  * @param sb
  *    The string buffer to print into.
  * @param oldpath
  *    The first pathname being checked.
  * @param oldpath_caption
  *    The argument name of the first pathname being checked.
  * @param oldpath_final_component
  *    The desired properties of the last component of oldpath
  * @param newpath
  *    The second pathname being checked.
  * @param newpath_caption
  *    The argument name of the second pathname being checked.
  * @param newpath_final_component
  *    The desired properties of the last component of newpath
  */
void explain_buffer_eloop2(explain_string_buffer_t *sb,
    const char *oldpath, const char *oldpath_caption,
    const struct explain_final_t *oldpath_final_component,
    const char *newpath, const char *newpath_caption,
    const struct explain_final_t *newpath_final_component);

#endif /* LIBEXPLAIN_BUFFER_ELOOP_H */
/* vim: set ts=8 sw=4 et : */
