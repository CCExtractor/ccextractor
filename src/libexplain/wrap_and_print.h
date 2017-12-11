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

#ifndef LIBEXPLAIN_WRAP_AND_PRINT_H
#define LIBEXPLAIN_WRAP_AND_PRINT_H

#include <libexplain/ac/stdio.h>

/**
  * The explain_wrap_and_print function may be used to take a piece
  * of text and wrap it to 80 columns and print it to the given output
  * stream.
  *
  * @param fp
  *    The file descriptor to write the output on.
  * @param text
  *    The text to be wrapped and printed.
  */
void explain_wrap_and_print(FILE *fp, const char *text);

/**
  * The explain_wrap_and_print_width function may be used to take a
  * piece of text and wrap it to the specified number of columns and
  * print it to the given output stream.
  *
  * @param fp
  *    The file descriptor to write the output on.
  * @param text
  *    The text to be wrapped and printed.
  * @param width
  *    The width of the page, counted in fixed-width character columns.
  */
void explain_wrap_and_print_width(FILE *fp, const char *text, int width);

#endif /* LIBEXPLAIN_WRAP_AND_PRINT_H */
/* vim: set ts=8 sw=4 et : */
