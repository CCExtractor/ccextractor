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

#ifndef LIBEXPLAIN_BUFFER_TIOCM_H
#define LIBEXPLAIN_BUFFER_TIOCM_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_tiocm function may be used to print the "modem
  * bits" arguments if the TIOCMGET, TIOCMSET, TIOCMBIC and TIOCMBIS
  * ioctl requests.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     The value of the bit-wise-or of the modem bits.
  */
void explain_buffer_tiocm(explain_string_buffer_t *sb, int value);

/**
  * The explain_buffer_tiocm_star function may be used to print the "modem
  * bits" arguments if the TIOCMGET, TIOCMSET, TIOCMBIC and TIOCMBIS
  * ioctl requests.
  *
  * @param sb
  *     The string buffer to print into.
  * @param value
  *     A pointer to the value of the bit-wise-or of the modem bits.
  */
void explain_buffer_tiocm_star(explain_string_buffer_t *sb,
    const int *value);

#endif /* LIBEXPLAIN_BUFFER_TIOCM_H */
/* vim: set ts=8 sw=4 et : */
