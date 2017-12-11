/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_RWX_H
#define LIBEXPLAIN_BUFFER_RWX_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_rwx function may be used to print a
  * representation of a "rwx" access mode.
  *
  * It prints 'r' if any of 0444 are set.
  * It prints 'w' if any of 0222 are set.
  * It prints 'x' if any of 0111 are set.
  *
  * @param sb
  *     The string buffer to print into.
  * @param mode
  *     The access mode value to be printed.
  */
void explain_buffer_rwx(explain_string_buffer_t *sb, int mode);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_RWX_H */
