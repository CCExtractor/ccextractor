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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_GENERIC_H
#define LIBEXPLAIN_BUFFER_ERRNO_GENERIC_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_errno_generic function is used to explain
  * errors for which a completely generic explanation is possible, and
  * which is more informative than strerror alone.
  *
  * @param sb
  *    The string bufferto print into.
  * @param errnum
  *    The errno value to explain
  * @param syscall_name
  *    The name of the problematic system call, for improving error
  *    messages.
  */
void explain_buffer_errno_generic(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_GENERIC_H */
/* vim: set ts=8 sw=4 et : */
