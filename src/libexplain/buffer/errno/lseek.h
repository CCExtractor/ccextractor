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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_LSEEK_H
#define LIBEXPLAIN_BUFFER_ERRNO_LSEEK_H

#include <libexplain/ac/sys/types.h>
#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_errno_lseek function may be used to obtain
  * a human readable explanation of what went wrong in an lseek(2)
  * system call.  The least the message will contain is the value of
  * strerror(errnum), but usually it will do much better, and indicate
  * the underlying cause in more detail.
  *
  * @param sb
  *    The string buffer into which the message is to be written.
  * @param errnum
  *    The error value to be decoded, usually obtain from the errno
  *    global variable just before this function is called.  This
  *    is necessary if you need to call <b>any</b> code between the
  *    system call to be explained and this function, because many libc
  *    functions will alter the value of errno.
  * @param fildes
  *    The file descriptor to seek on,
  *    exactly as passed to the lseek(2) system call.
  * @param offset
  *    exactly as passed to the lseek(2) system call.
  * @param whence
  *    exactly as passed to the lseek(2) system call.
  * @note
  *    Given a suitably thread safe buffer, this function is thread safe.
  */
void explain_buffer_errno_lseek(struct explain_string_buffer_t *sb,
    int errnum, int fildes, off_t offset, int whence);


/**
  * The explain_buffer_errno_lseek_explanation function is called by the
  * explain_buffer_errno_lseek function (and others) to print the
  * explanation, the part after "because..."
  *
  * @param sb
  *    The string buffer into which the message is to be written.
  * @param errnum
  *    The error value to be decoded.
  * @param syscall_name
  *     The name of the offending system call.
  * @param fildes
  *    The file descriptor to seek on,
  *    exactly as passed to the lseek(2) system call.
  * @param offset
  *    exactly as passed to the lseek(2) system call.
  * @param whence
  *    exactly as passed to the lseek(2) system call.
  * @note
  *    Given a suitably thread safe buffer, this function is thread safe.
  */
void explain_buffer_errno_lseek_explanation(struct explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, off_t offset, int whence);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_LSEEK_H */
/* vim: set ts=8 sw=4 et : */
