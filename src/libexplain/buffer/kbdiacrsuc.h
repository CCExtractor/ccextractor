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

#ifndef LIBEXPLAIN_BUFFER_KBDIACRSUC_H
#define LIBEXPLAIN_BUFFER_KBDIACRSUC_H

#include <libexplain/string_buffer.h>

struct kbdiacrsuc; /* forward */

/**
  * The explain_buffer_kbdiacrsuc function may be used to
  * print a representation of a kbdiacrsuc structure value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The kbdiacrsuc structure vale to be printed.
  * @param extra
  *     if to print entries as well
  */
void explain_buffer_kbdiacrsuc(explain_string_buffer_t *sb,
    const struct kbdiacrsuc *value, int extra);

#endif /* LIBEXPLAIN_BUFFER_KBDIACRSUC_H */
/* vim: set ts=8 sw=4 et : */
