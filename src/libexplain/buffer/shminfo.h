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

#ifndef LIBEXPLAIN_BUFFER_SHMINFO_H
#define LIBEXPLAIN_BUFFER_SHMINFO_H

#include <libexplain/string_buffer.h>

struct shminfo; /* forward */

/**
  * The explain_buffer_shminfo function may be used
  * to print a representation of a shminfo structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The shminfo structure to be printed.
  */
void explain_buffer_shminfo(explain_string_buffer_t *sb,
    const struct shminfo *data);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_SHMINFO_H */
