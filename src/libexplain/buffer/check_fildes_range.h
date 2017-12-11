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

#ifndef LIBEXPLAIN_BUFFER_CHECK_FILDES_RANGE_H
#define LIBEXPLAIN_BUFFER_CHECK_FILDES_RANGE_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_check_fildes_range function may be used to
  * check that a file descriptor is within the acceptable range, and
  * print an explanation if it is not.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The file descriptor value to be checked.
  * @param caption
  *    The name of the function call argument containg the file
  *    descriptor value to be checked.
  * @returns
  *    0 if printed an explanation,
  *    -1 if not out of range (no explanation printed)
  */
int explain_buffer_check_fildes_range(explain_string_buffer_t *sb,
    int fildes, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_CHECK_FILDES_RANGE_H */
/* vim: set ts=8 sw=4 et : */
