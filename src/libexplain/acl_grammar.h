/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_ACL_GRAMMAR_H
#define LIBEXPLAIN_ACL_GRAMMAR_H

/**
  * The explain_acl_parse function is
  * called when we already know the string is unparse-able.
  * So the idea is to accumulate a better explanation than just EINVAL.
  *
  * @param text
  *    The ACL string passed to acl_from_text(3).
  * @returns
  *    pointer to an explanatory string.
  *    Or the NULL pointer of the string is perfectly acceptable.
  */
char *explain_acl_parse(const char *text);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_ACL_GRAMMAR_H */
