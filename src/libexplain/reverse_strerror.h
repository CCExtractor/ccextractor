/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_REVERSE_STRERROR_H
#define LIBEXPLAIN_REVERSE_STRERROR_H

/**
  * The explain_reverse_strerror function may be used to translate
  * the text of an error returned by strerror back into an errno value.
  * Given that useres frequently mangle the text when they are reporting
  * error messages, this can be tricky.
  *
  * @param text
  *    The text of the error message.
  * @returns
  *    an errno value, or -1 if there is nothing remotely similar
  */
int explain_reverse_strerror(const char *text);

#endif /* LIBEXPLAIN_REVERSE_STRERROR_H */
/* vim: set ts=8 sw=4 et : */
