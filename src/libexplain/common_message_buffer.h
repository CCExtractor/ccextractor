/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_COMMON_MESSAGE_BUFFER_H
#define LIBEXPLAIN_COMMON_MESSAGE_BUFFER_H

/**
  * The explain_common_message_buffer global variable is used to
  * store the return message by all functiosn which the user does not
  * supply with an explict return buffer.  This common message buffer is
  * shared amongst all such functions.  This renders all such functions
  * non-thread-safe.
  */
extern char explain_common_message_buffer[];

/**
  * The explain_common_message_buffer_size global variable is used to
  * remember the size of the #explain_common_message_buffer array.
  */
extern const unsigned explain_common_message_buffer_size;

#endif /* LIBEXPLAIN_COMMON_MESSAGE_BUFFER_H */
/* vim: set ts=8 sw=4 et : */
