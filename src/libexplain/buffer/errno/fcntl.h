/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_FCNTL_H
#define LIBEXPLAIN_BUFFER_ERRNO_FCNTL_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_errno_fcntl function is used to obtain an
  * explanation of an error returned by the fcntl(2) system call.  The
  * least the message will contain is the value of strerror(errnum), but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * @param sb
  *     The string buffer in which the message is being constructed.
  *     If a safe buffer is specified, this function is thread safe.
  * @param errnum
  *     The error value to be decoded, usually obtain from the errno
  *     global variable just before this function is called.  This
  *     is necessary if you need to call <b>any</b> code between the
  *     system call to be explained and this function, because many libc
  *     functions will alter the value of errno.
  * @param fildes
  *     The original fildes, exactly has passed to the fcntl(2) system call.
  * @param command
  *     The original command, exactly has passed to the fcntl(2) system call.
  * @param arg
  *     The original arg, exactly has passed to the fcntl(2) system call
  *     (or zero if the original call didn't need a mode argument, or
  *     cast to long if it was a pointer).
  */
void explain_buffer_errno_fcntl(struct explain_string_buffer_t *sb,
    int errnum, int fildes, int command, long arg);

/**
  * The explain_fcntl_command_parse function may be used to parse a text
  * representation of a fcntl command argument, and return a numeric value.
  * Prints error message and exits if string cannot be parsed.
  *
  * @param text
  *     The text to be parsed.
  * @param captn
  *     Additional caption to be added to error message, if needed.
  * @returns
  *     integer value of command
  */
int explain_fcntl_command_parse_or_die(const char *text, const char *captn);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_FCNTL_H */
/* vim: set ts=8 sw=4 et : */
