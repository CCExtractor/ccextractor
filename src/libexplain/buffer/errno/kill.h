/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_ERRNO_KILL_H
#define LIBEXPLAIN_BUFFER_ERRNO_KILL_H

#include <libexplain/ac/signal.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_errno_kill function is used to obtain an explanation
  * of an error returned by the <i>kill</i>(2) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * @param sb
  *     The string buffer to print the message into. If a suitable buffer
  *     is specified, this function is thread safe.
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param pid
  *     The original pid, exactly as passed to the <i>kill</i>(2) system
  *     call.
  * @param sig
  *     The original sig, exactly as passed to the <i>kill</i>(2) system
  *     call.
  */
void explain_buffer_errno_kill(explain_string_buffer_t *sb, int errnum, pid_t
    pid, int sig);

/**
  * The explain_buffer_errno_kill_explanation function is used to print
  * an explanation (the text after "basecase") of an error returned by
  * the <i>kill</i>(2) system call.
  *
  * @param sb
  *     The string buffer to print the message into.
  * @param errnum
  *     The error value to be decoded.
  * @param syscall_name
  *     The name of the offending system call.
  * @param pid
  *     The original pid, exactly as passed to the system call.
  * @param sig
  *     The original sig, exactly as passed to the system call.
  */
void explain_buffer_errno_kill_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, pid_t pid, int sig);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_ERRNO_KILL_H */
