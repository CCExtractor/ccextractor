/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ENOSYS_H
#define LIBEXPLAIN_BUFFER_ENOSYS_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_enosys_fildes function may be used to print an
  * explanation of an ENOSYS or EOPNOTSUPP error, associated with a
  * particular file descriptor (device?).
  *
  * @param sb
  *     The string buffer to print into.
  * @param fildes
  *     The offending file descriptor.
  * @param caption
  *     The name of the offending system call argument.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_enosys_fildes(explain_string_buffer_t *sb, int fildes,
    const char *caption, const char *syscall_name);

/**
  * The explain_buffer_enosys_pathname function may be used to print
  * an explanation of an ENOSYS or EOPNOTSUPP error, associated with a
  * particular pathname (device?).
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The offending file path.
  * @param caption
  *     The name of the offending system call argument.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_enosys_pathname(explain_string_buffer_t *sb,
    const char *pathname, const char *caption, const char *syscall_name);

/**
  * The explain_buffer_enosys_socket function may be used to print an
  * explanation of an ENOSYS or EOPNOTSUPP error, associated with a
  * particular network socket.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offending system call.
  * @param fildes
  *     The offending file descriptor.
  */
void explain_buffer_enosys_socket(explain_string_buffer_t *sb,
    const char *syscall_name, int fildes);

/**
  * The explain_buffer_enosys_vague function may be used to print an
  * explanation of an ENOSYS or EOPNOTSUPP error.  This is a generic
  * message, use a more specific one if at all possible.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_enosys_vague(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_enosys_acl function may be used to print an
  * explanation of an ENOSYS or EOPNOTSUPP error.  In the cae where it
  * one of the Access Control List (ACL) functions.
  *
  * @param sb
  *     The string buffer to print into.
  * @param caption
  *     The name of the offending system call argument.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_enosys_acl(explain_string_buffer_t *sb, const char *caption,
    const char *syscall_name);

#endif /* LIBEXPLAIN_BUFFER_ENOSYS_H */
/* vim: set ts=8 sw=4 et : */
