/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_BLK_USER_TRACE_SETUP_H
#define LIBEXPLAIN_BUFFER_BLK_USER_TRACE_SETUP_H

#include <libexplain/string_buffer.h>

struct blk_user_trace_setup; /* forward */

/**
  * The explain_buffer_blk_user_trace_setup function may be used
  * to print a representation of a blk_user_trace_setup structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The blk_user_trace_setup structure to be printed.
  */
void explain_buffer_blk_user_trace_setup(explain_string_buffer_t *sb,
    const struct blk_user_trace_setup *data);

#endif /* LIBEXPLAIN_BUFFER_BLK_USER_TRACE_SETUP_H */
/* vim: set ts=8 sw=4 et : */
