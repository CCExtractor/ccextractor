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

#ifndef LIBEXPLAIN_BUFFER_LINE_DISCIPLINE_H
#define LIBEXPLAIN_BUFFER_LINE_DISCIPLINE_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_line_discipline function may be used to
  * print a representation of a line discipline.
  *
  * @param sb
  *    The string buffer to print into.
  * @param arg
  *    the line discipline of interest
  */
void explain_buffer_line_discipline(explain_string_buffer_t *sb, int arg);

/**
  * The explain_buffer_line_discipline_star function may be used to
  * print a representation of a line discipline.
  *
  * @param sb
  *    The string buffer to print into.
  * @param arg
  *    a pointer to the line discipline of interest
  */
void explain_buffer_line_discipline_star(explain_string_buffer_t *sb,
    const int *arg);

#endif /* LIBEXPLAIN_BUFFER_LINE_DISCIPLINE_H */
/* vim: set ts=8 sw=4 et : */
