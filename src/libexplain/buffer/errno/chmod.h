/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_CHMOD_H
#define LIBEXPLAIN_BUFFER_ERRNO_CHMOD_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_errno_chmod function may be used to obtain a
  * detailed explanation of an error returned by a chmod(2) system call.
  *
  * @param sb
  *    The string buffer to print the explanation into.
  * @param errnum
  *    The returned error, usually obtained from the errno global variable.
  * @param pathname
  *    The original pathname, exactly as passed to the chmod(2) system call.
  * @param mode
  *    The original mode, acactly as passed to the chmod(2) system call.
  */
void explain_buffer_errno_chmod(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int mode);

/**
  * The explain_buffer_errno_chmod_explanation_fc function contaisn
  * the code common to both explain_buffer_errno_chmod_explanation and
  * explain_buffer_errno_lchmod_explanation.
  *
  * @param sb
  *    The string buffer to print the explanation into.
  * @param errnum
  *    The returned error, usually obtained from the errno global variable.
  * @param syscall_name
  *    The name of the system call being explained.
  * @param pathname
  *    The original pathname, exactly as passed to the chmod(2) system call.
  * @param mode
  *    The original mode, acactly as passed to the chmod(2) system call.
  * @param final_component
  *    The expected properties of the final path component.
  */
void
explain_buffer_errno_chmod_explanation_fc(explain_string_buffer_t *sb,
    int errnum,  const char *syscall_name, const char *pathname, int mode,
    const struct explain_final_t *final_component);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_CHMOD_H */
/* vim: set ts=8 sw=4 et : */
