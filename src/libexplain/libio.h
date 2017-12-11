/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_LIBIO_H
#define LIBEXPLAIN_LIBIO_H

#include <libexplain/ac/stdio.h>

/**
  * The explain_libio_no_reads function may be used to determine whether
  * or not the given i/o stream may be read from.
  *
  * @param fp
  *    The i/o stream to check for readability.
  * @returns
  *    false (zero) if the stream may be read,
  *    true (non-zero) if the stream may not be read.
  */
int explain_libio_no_reads(FILE *fp);

/**
  * The explain_libio_no_writes function may be used to determine
  * whether or not the given i/o stream may be read from.
  *
  * @param fp
  *    The i/o stream to check for readability.
  * @returns
  *    false (zero) if the stream may be written,
  *    true (non-zero) if the stream may not be written.
  */
int explain_libio_no_writes(FILE *fp);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_LIBIO_H */
