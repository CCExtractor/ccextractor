/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_CHOWN_H
#define LIBEXPLAIN_BUFFER_ERRNO_CHOWN_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_errno_chown function
  * is used to obtain an explanation of an error returned
  * by the chown(2) system call.
  * The least the message will contain is the value of
  * strerror(errnum), but usually it will do much better,
  * and indicate the underlying cause in more detail.
  *
  * @param sb
  *     The string buffer to print the message into.  If a
  *     safe buffer is specified, this function is thread
  *     safe.
  * @param errnum
  *     The error value to be decoded, usually obtained
  *     from the errno global variable just before this
  *     function is called.  This is necessary if you need
  *     to call <b>any</b> code between the system call to
  *     be explained and this function, because many libc
  *     functions will alter the value of errno.
  * @param pathname
  *     The original pathname, exactly as passed to the chown(2) system call.
  * @param uid
  *     The original owner, exactly as passed to the chown(2) system call.
  * @param gid
  *     The original group, exactly as passed to the chown(2) system call.
  */
void explain_buffer_errno_chown(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int uid, int gid);

struct explain_final_t; /* forward */

/**
  * The explain_buffer_errno_chown_explanation_fc function factors out
  * code common to both the explain_buffer_errno_chown_explanation and
  * explain_buffer_errno_lchown_explanation functions.
  *
  * @param sb
  *     The string buffer to print the message into.
  * @param errnum
  *     The error value to be decoded.
  * @param syscall_name
  *     The name of the offending system call.
  * @param pathname
  *     The original pathname, exactly as passed to the chown(2) system call.
  * @param uid
  *     The original owner, exactly as passed to the chown(2) system call.
  * @param gid
  *     The original group, exactly as passed to the chown(2) system call.
  * @param pathname_caption
  *     The name of the offending syscall argument.
  * @param final_component
  *     The designed properties of the final component.
  */
void explain_buffer_errno_chown_explanation_fc(
    explain_string_buffer_t *sb, int errnum, const char *syscall_name,
    const char *pathname, int uid, int gid,
    const char *pathname_caption,
    struct explain_final_t *final_component);

/**
  * The explain_buffer_errno_fchown_explanation
  * function factors out code common to both it and the
  * explain_buffer_errno_chown_explanation function.
  *
  * @param sb
  *     The string buffer to print the message into.
  * @param errnum
  *     The error value to be decoded.
  * @param syscall_name
  *     The name of the offending system call.
  * @param fildes
  *     The original fildes, exactly as passed to the fchown(2) system call.
  * @param uid
  *     The original owner, exactly as passed to the fchown(2) system call.
  * @param gid
  *     The original group, exactly as passed to the fchown(2) system call.
  * @param fildes_caption
  *     The name of the offending syscall argument.
  */
void explain_buffer_errno_fchown_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, int uid, int gid,
    const char *fildes_caption);

/**
  * The explain_buffer_errno_fchown_explanation function factors out
  * code common to both several hown systcall foms.
  *
  * @param sb
  *     The string buffer to print the message into.
  * @param errnum
  *     The error value to be decoded.
  * @param syscall_name
  *     The name of the offending system call.
  * @param pathname
  *     The original fildes, exactly as passed to the fchown(2) system call.
  * @param owner
  *     The original owner, exactly as passed to the fchown(2) system call.
  * @param group
  *     The original group, exactly as passed to the fchown(2) system call.
  * @param pathname_caption
  *     The name of the offending system call argument.
  */
void explain_buffer_errno_chown_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname, int owner,
    int group, const char *pathname_caption);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_CHOWN_H */
/* vim: set ts=8 sw=4 et : */
