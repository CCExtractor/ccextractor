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

#ifndef LIBEXPLAIN_BUFFER_CONSOLE_FONT_OP_H
#define LIBEXPLAIN_BUFFER_CONSOLE_FONT_OP_H

#include <libexplain/string_buffer.h>

struct console_font_op; /* forward */

/**
  * The explain_buffer_console_font_op function may be used to
  * print a representation of a console_font_op structure value.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The console_font_op structure value to be printed.
  */
void explain_buffer_console_font_op(explain_string_buffer_t *sb,
    const struct console_font_op *value);

#endif /* LIBEXPLAIN_BUFFER_CONSOLE_FONT_OP_H */
/* vim: set ts=8 sw=4 et : */
