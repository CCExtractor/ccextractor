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

#ifndef LIBEXPLAIN_BUFFER_WAIT_STATUS_H
#define LIBEXPLAIN_BUFFER_WAIT_STATUS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_wait_status function may be used to decode a
  * status value returned by wait(2) et al, and print it on the given
  * string buffer.
  *
  * @param sb
  *    The string buffer to print into
  * @param status
  *    The status value to decode.  See wait(2) for more information.
  */
void explain_buffer_wait_status(explain_string_buffer_t *sb, int status);

#endif /* LIBEXPLAIN_BUFFER_WAIT_STATUS_H */
/* vim: set ts=8 sw=4 et : */
