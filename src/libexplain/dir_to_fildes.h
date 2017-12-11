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

#ifndef LIBEXPLAIN_DIR_TO_FILDES_H
#define LIBEXPLAIN_DIR_TO_FILDES_H

#include <libexplain/ac/dirent.h>

/**
  * The explain_dir_to_fildes function may be used to extract the
  * file descriptor from a DIR pointer, but carefully.
  *
  * @param dir
  *    The DIR of interest
  * @returns
  *    file descriptor, or -1 on error
  */
int explain_dir_to_fildes(DIR *dir);

#endif /* LIBEXPLAIN_DIR_TO_FILDES_H */
/* vim: set ts=8 sw=4 et : */
