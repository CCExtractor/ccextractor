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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_READ_H
#define LIBEXPLAIN_BUFFER_ERRNO_READ_H

#include <libexplain/string_buffer.h>

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_errno_lseek_explanation function is called by the
  * explain_buffer_errno_lseek function (and others) to print the
  * explanation, the part after "because..."
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
  *    The file descriptor to be read from,
  *    exactly as passed to the read(2) system call.
  * @param data
  *    The address of the base address in memory to write the data
  *    (the original read call modified the data, this function will not),
  *    exactly as passed to the read(2) system call.
  * @param data_size
  *    The maximum number of bytes of data to be read,
  *    exactly as passed to the read(2) system call.
  * @note
  *    Given a suitably thread safe buffer, this function is thread safe.
  */
void explain_buffer_errno_read(explain_string_buffer_t *sb,
    int errnum, int fildes, const void *data, size_t data_size);

/**
  * The explain_buffer_errno_read_explanation function is called by the
  * explain_buffer_errno_read function (and others) to print the
  * explanation, the part after "because..."
  *
  * @param sb
  *    The string buffer into which the message is to be written.
  * @param errnum
  *    The error value to be decoded, usually obtain from the errno
  *    global variable just before this function is called.  This
  *    is necessary if you need to call <b>any</b> code between the
  *    system call to be explained and this function, because many libc
  *    functions will alter the value of errno.
  * @param syscall_name
  *    The name of the offending system call.
  * @param fildes
  *    The file descriptor to be read from,
  *    exactly as passed to the read(2) system call.
  * @param data
  *    The address of the base address in memory to write the data
  *    (the original read call modified the data, this function will not),
  *    exactly as passed to the read(2) system call.
  * @param data_size
  *    The maximum number of bytes of data to be read,
  *    exactly as passed to the read(2) system call.
  * @note
  *    Given a suitably thread safe buffer, this function is thread safe.
  */
void explain_buffer_errno_read_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, const void *data,
    size_t data_size);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_READ_H */
/* vim: set ts=8 sw=4 et : */
