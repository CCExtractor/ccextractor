/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_EEXIST_H
#define LIBEXPLAIN_BUFFER_EEXIST_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_eexist function may be used to print an
  * explanation of an EEXIST error, reported by the open, mkdir, link
  * and synlink system calls.
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The offending file system path.
  */
void explain_buffer_eexist(explain_string_buffer_t *sb, const char *pathname);

/**
  * The explain_buffer_eexist5 function may be used to print an
  * explanation of an EEXIST error, reported by the open system call
  * (and indirectly by others via the explain_buffer_eexist function).
  *
  * @param sb
  *     The string buffer to print into.
  * @param base_name
  *     The offending final path component
  * @param base_mode
  *     The st_mode field of the offending final path component
  * @param dir_name
  *     The directory containing the offending path component.
  * @param dir_mode
  *     The st_mode field of the directory containing the offending path
  *     component.
  */
void explain_buffer_eexist5(explain_string_buffer_t *sb, const char *base_name,
    int base_mode, const char *dir_name, int dir_mode);

/**
  * The explain_buffer_eexist_tempname function is used to explain an
  * EEXIST error, reported by the __gen_tempname function (indirectly
  * used by many of the [e]glibc functions that generate temporary file
  * names), when it can't find an unused (nonexistant) temporary file name.
  *
  * @param sb
  *     The string buffer to print into.
  * @param directory
  *     The problematic directory.
  */
void explain_buffer_eexist_tempname(explain_string_buffer_t *sb,
    const char *directory);

/**
  * The explain_buffer_eexist_tempname function is used to explain an
  * EEXIST error, reported by the __gen_tempname function (indirectly
  * used by many of the [e]glibc functions that generate temporary file
  * names), when it can't find an unused (nonexistant) temporary file name.
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The problematic pathname (the directory will be extracted).
  */
void explain_buffer_eexist_tempname_dirname(explain_string_buffer_t *sb,
    const char *pathname);

#endif /* LIBEXPLAIN_BUFFER_EEXIST_H */
/* vim: set ts=8 sw=4 et : */
