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

#ifndef LIBEXPLAIN_STREAM_TO_FILDES_H
#define LIBEXPLAIN_STREAM_TO_FILDES_H

#include <libexplain/ac/stdio.h>

/**
  * The explain_stream_to_fildes function may be used to obtain a
  * file descriptor from a stream pointer.  Care is taken to avoid
  * segfaults, hence this wrapper around fileno(3).
  *
  * @param fp
  *    The stream of interest
  * @returns
  *    file descriptor, or -1 if unsuccessful
  */
int explain_stream_to_fildes(FILE *fp);

#endif /* LIBEXPLAIN_STREAM_TO_FILDES_H */
/* vim: set ts=8 sw=4 et : */
