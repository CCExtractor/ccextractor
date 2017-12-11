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

#ifndef LIBEXPLAIN_BUFFER_DIR_TO_PATHNAME_H
#define LIBEXPLAIN_BUFFER_DIR_TO_PATHNAME_H

#include <libexplain/ac/dirent.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_dir_to_pathname function may be used to
  * extract the file descriptor from the gibe DIR pointer, and print the
  * name of the associated file.
  *
  * @param sb
  *    the string buffer to print into
  * @param dir
  *    the DIR pointer of interest
  */
void explain_buffer_dir_to_pathname(explain_string_buffer_t *sb,
    DIR *dir);

#endif /* LIBEXPLAIN_BUFFER_DIR_TO_PATHNAME_H */
/* vim: set ts=8 sw=4 et : */
