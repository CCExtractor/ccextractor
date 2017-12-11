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

#ifndef LIBEXPLAIN_BUFFER_EROFS_H
#define LIBEXPLAIN_BUFFER_EROFS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_erofs function may be used to
  * report EROFS errors.
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The path name of the pffending file.
  * @param caption
  *    The name of the parameter of the path name of the pffending file.
  */
void explain_buffer_erofs(explain_string_buffer_t *sb,
    const char *pathname, const char *caption);

/**
  * The explain_buffer_erofs function may be used to
  * report EROFS errors, given a file descriptor.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The file descriptor of the pffending file.
  * @param caption
  *    The name of the parameter of the file descriptor of the offending file.
  */
void explain_buffer_erofs_fildes(explain_string_buffer_t *sb, int fildes,
    const char *caption);

#endif /* LIBEXPLAIN_BUFFER_EROFS_H */
/* vim: set ts=8 sw=4 et : */
