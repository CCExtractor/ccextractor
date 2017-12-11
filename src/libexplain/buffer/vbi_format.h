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

#ifndef LIBEXPLAIN_BUFFER_VBI_FORMAT_H
#define LIBEXPLAIN_BUFFER_VBI_FORMAT_H

#include <libexplain/string_buffer.h>

struct vbi_format; /* forward */

/**
  * The explain_buffer_vbi_format function may be used
  * to print a representation of a vbi_format structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The vbi_format structure to be printed.
  */
void explain_buffer_vbi_format(explain_string_buffer_t *sb,
    const struct vbi_format *data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_VBI_FORMAT_H */
