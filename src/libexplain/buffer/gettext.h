/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_GETTEXT_H
#define LIBEXPLAIN_BUFFER_GETTEXT_H

#include <libexplain/string_buffer.h>

#ifndef i18n
#define i18n(x) x
#endif

/**
  * The explain_buffer_gettext function may be used to translate a
  * string via gettext, and then print it on the given string buffer.
  *
  * @param sb
  *     The string buffer to print on.
  * @param str
  *    The string to be localized and printed.
  */
void explain_buffer_gettext(explain_string_buffer_t *sb, const char *str);

/**
  * The explain_buffer_gettext_printf function may be used to
  * translate a string via gettext, and then use it as the format string
  * for a printf into the given string buffer.
  *
  * @param sb
  *     The string buffer to print on.
  * @param fmt
  *    The format string describing the rest of the arguments, to be
  *    localized and printed.
  */
void explain_buffer_gettext_printf(explain_string_buffer_t *sb,
    const char *fmt, ...)
                                                 LIBEXPLAIN_FORMAT_PRINTF(2, 3);

#endif /* LIBEXPLAIN_BUFFER_GETTEXT_H */
/* vim: set ts=8 sw=4 et : */
