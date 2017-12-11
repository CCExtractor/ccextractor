/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_DANGEROUS_H
#define LIBEXPLAIN_BUFFER_DANGEROUS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_dangerous function may be used to print an
  * reminder that the given system call is dangerous, and a more secure
  * alternative should be used.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the dangerous system call.
  */
void explain_buffer_dangerous_system_call(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_dangerous function may be used to print an
  * reminder that the given system call is dangerous, and a more secure
  * alternative should be used.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the dangerous system call.
  * @param suggested_alternative
  *     The name of the less dangerous alternative system call.
  */
void explain_buffer_dangerous_system_call2(explain_string_buffer_t *sb,
    const char *syscall_name, const char *suggested_alternative);

#endif /* LIBEXPLAIN_BUFFER_DANGEROUS_H */
/* vim: set ts=8 sw=4 et : */
