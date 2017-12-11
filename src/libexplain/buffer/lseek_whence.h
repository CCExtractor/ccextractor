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

#ifndef LIBEXPLAIN_BUFFER_LSEEK_WHENCE_H
#define LIBEXPLAIN_BUFFER_LSEEK_WHENCE_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_lseek_whence function may be used to form a
  * human readable representation of an lseek when value.
  *
  * @param sb
  *    the string buffer in which to write the whens symbol
  * @param whence
  *    the value to be decoded
  */
void explain_buffer_lseek_whence(struct explain_string_buffer_t *sb,
    int whence);

/**
  * The explain_lseek_whence_parse function may be used to parse
  * a text string into an lseek whence value.  It may be symbolic or
  * numeric.  If an error occurs, will print a diagnostic and exit.
  *
  * @param text
  *     The text string to be parsed.
  * @param caption
  *     an additional caption to add to the error message.
  * @returns
  *     an lseek whence value
  */
int explain_lseek_whence_parse_or_die(const char *text, const char *caption);

#endif /* LIBEXPLAIN_BUFFER_LSEEK_WHENCE_H */
/* vim: set ts=8 sw=4 et : */
