/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_FSTRCMP_H
#define LIBEXPLAIN_FSTRCMP_H

/**
  * The explain_fstrcmp function may be used to compare two strings.
  * The order of the strings has no effect on the result.
  *
  * @param s1
  *     The first string to be compared.
  * @param s2
  *     The second string to be compared.
  * @returns
  *     Rather than a simple yes/no result, the similarity of the
  *     strings is returned.  A return of 0.0 means the strings are
  *     completely differemt, and a return value of 1.0 means they are
  *     identical.  However, values can be returned between these two
  *     values, indicating the degree of similarity.
  * @note
  *     This function uses dynamic memory.
  *     May return -1 if malloc fails.
  *     This function is not thread-safe.
  */
double explain_fstrcmp(const char *s1, const char *s2);

/**
  * The explain_fstrcasecmp function may be used to compare two strings.  The
  * comparison is case <b>in</b>sensitive.  The order of the strings has no
  * effect on the result.
  *
  * @param s1
  *     The first string to be compared.
  * @param s2
  *     The second string to be compared.
  * @returns
  *     Rather than a simple yes/no result, the similarity of the
  *     strings is returned.  A return of 0.0 means the strings are
  *     completely differemt, and a return value of 1.0 means they are
  *     identical.  However, values can be returned between these two
  *     values, indicating the degree of similarity.
  * @note
  *     This function uses dynamic memory.
  *     May return -1 if malloc fails.
  */
double explain_fstrcasecmp(const char *s1, const char *s2);

#endif /* LIBEXPLAIN_FSTRCMP_H */
/* vim: set ts=8 sw=4 et : */
