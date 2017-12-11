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

#ifndef LIBEXPLAIN_BUFFER_EISDIR_H
#define LIBEXPLAIN_BUFFER_EISDIR_H

#include <libexplain/string_buffer.h>

struct eisdir; /* forward */

/**
  * The explain_buffer_eisdir function may be used to print an
  * explainaton for an EISDIR error, in the case where a file was to be
  * opened for writing.
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The pathname of the offending file.
  * @param caption
  *     The name of the offending function call parameter.
  * @returns
  *     bool; true (non-zero) if a message was issued, false (zero) if
  *     no message was issued.
  */
int explain_buffer_eisdir(explain_string_buffer_t *sb, const char *pathname,
    const char *caption);

#endif /* LIBEXPLAIN_BUFFER_EISDIR_H */
/* vim: set ts=8 sw=4 et : */
