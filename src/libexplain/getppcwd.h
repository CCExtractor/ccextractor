/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_GETPPCWD_H
#define LIBEXPLAIN_GETPPCWD_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_getppcwd function may be used to obtain the current
  * directory of the parent process.
  *
  * @param data
  *    Where to put the path of the parent process' current working directory.
  * @param data_size
  *    The available room in the data bufer.
  * @returns
  *    NULL if fails, non-NULL if succeeds
  */
char *explain_getppcwd(char *data, size_t data_size);

#endif /* LIBEXPLAIN_GETPPCWD_H */
/* vim: set ts=8 sw=4 et : */
