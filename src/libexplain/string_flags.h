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

#ifndef LIBEXPLAIN_STRING_FLAGS_H
#define LIBEXPLAIN_STRING_FLAGS_H

typedef struct explain_string_flags_t explain_string_flags_t;
struct explain_string_flags_t
{
    int             flags_string_valid;
    int             flags;
    int             rwa_seen;
    char            invalid[50];
};

/**
  * The explain_string_flags_init function may be used to translate an
  * fopen(3) flags string argument into an open(2) flags bit field
  * argument.
  *
  * @param sf
  *    The string flags to fill
  * @param from
  *    the string to fill the string flags from
  */
void explain_string_flags_init(explain_string_flags_t *sf,
    const char *from);

/**
  * The explain_string_flags_einval function may be used to print an
  * expanation for an EINVAL error relating to the given string flags.
  *
  * @param sf
  *     The string flags of interest.
  * @param sb
  *     The string buffer to print into.
  * @param caption
  *     The name of th offending system call argument.
  */
void explain_string_flags_einval(const explain_string_flags_t *sf,
    explain_string_buffer_t *sb, const char *caption);

#endif /* LIBEXPLAIN_STRING_FLAGS_H */
/* vim: set ts=8 sw=4 et : */
