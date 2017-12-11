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

#ifndef LIBEXPLAIN_BUFFER_ETXTBSY_H
#define LIBEXPLAIN_BUFFER_ETXTBSY_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_etxtbsy function may be used to provide a
  * common explanation for all cases of ETXTBSY.
  *
  * @param sb
  *    The string buffer to print into
  * @param pathname
  *    The name of the file having the problem (it is not printed, it is
  *    used to look for the process using it)
  */
void explain_buffer_etxtbsy(explain_string_buffer_t *sb,
    const char *pathname);

/**
  * The explain_buffer_etxtbsy_fildes function may be used to
  * provide a common explanation for all cases of ETXTBSY, where a file
  * descriptor is available, rather than a pathname.
  *
  * @param sb
  *    The string buffer to print into
  * @param fildes
  *    The file descriptor having the problem (it is not printed, it is
  *    used to look for the process using it)
  */
void explain_buffer_etxtbsy_fildes(explain_string_buffer_t *sb,
    int fildes);

#endif /* LIBEXPLAIN_BUFFER_ETXTBSY_H */
/* vim: set ts=8 sw=4 et : */
