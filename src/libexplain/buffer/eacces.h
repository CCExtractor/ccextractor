/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_EACCES_H
#define LIBEXPLAIN_BUFFER_EACCES_H

#include <libexplain/string_buffer.h>

struct explain_final_t; /* forward */

/**
  * The explain_buffer_eacces function may be used to
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The problematic pathname
  * @param caption
  *    The name of the offending system call argument.
  * @param final_component
  *    The desired properties of the final component
  */
void explain_buffer_eacces(explain_string_buffer_t *sb,
    const char *pathname, const char *caption,
    const struct explain_final_t *final_component);

/**
  * The explain_buffer_eacces_syscall function may be used to
  * explan an EACCES error returned from a given system call.
  *
  * @param sb
  *    The string buffer to print into.
  * @param syscall_name
  *    The name of the offending system call argument.
  */
void explain_buffer_eacces_syscall(explain_string_buffer_t *sb,
    const char *syscall_name);

struct ipc_perm; /* forward */

/**
  * The explain_buffer_eacces_shmat function is use dto explain EACCES
  * error returned by shmat (and friends?).
  *
  * @param sb
  *     The string buffer to print into.
  * @param perm
  *     The access permission attached to the shared memory segment.
  * @param read_only
  *     Whether or no read only access was requested (if false,
  *     read-write access is requested).
  * @returns
  *     0 if it printed nothing, 1 if it printed an explanation
  */
int explain_buffer_eacces_shmat(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, int read_only);

/**
  * The explain_buffer_eacces_shmat_vague function is use dto explain EACCES
  * error returned by shmat, when no specific error can be determined.
  *
  * @param sb
  *     The string buffer to print into.
  */
void explain_buffer_eacces_shmat_vague(explain_string_buffer_t *sb);

#endif /* LIBEXPLAIN_BUFFER_EACCES_H */
/* vim: set ts=8 sw=4 et : */
