/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_FILENAME_H
#define LIBEXPLAIN_FILENAME_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_filename_from_stream function may be used to obtain the
  * filename associated with the given I/O stream.
  *
  * @param stream
  *     The file stream from which to obtain the filename.
  * @param data
  *     the filename will be returned in this buffer, NUL terminated.
  * @param data_size
  *     The available size of the return buffer, in bytes.
  *     If too small, the result will be silently truncated.
  * @returns
  *     0 on success, -1 on error (does NOT set errno)
  */
int explain_filename_from_stream(FILE *stream, char *data, size_t data_size);

/**
  * The explain_filename_from_stream function may be used to obtain the
  * filename associated with the given I/O stream.
  *
  * @param fildes
  *     The file descriptor from which to obtain the filename.
  * @param data
  *     the filename will be returned in this buffer, NUL terminated.
  * @param data_size
  *     The available size of the return buffer, in bytes.
  *     If too small, the result will be silently truncated.
  * @returns
  *     0 on success, -1 on error (does NOT set errno)
  */
int explain_filename_from_fildes(int fildes, char *data, size_t data_size);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_FILENAME_H */
