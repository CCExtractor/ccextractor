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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_SYMLINK_H
#define LIBEXPLAIN_BUFFER_ERRNO_SYMLINK_H

struct explain_string_buffer_t; /* forward */

/**
  * The explain_buffer_errno_symlink function is used to obtain an
  * explanation of an error returned by the symlink(2) system call.  The
  * least the message will contain is the value of strerror(errnum), but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * @param sb
  *     The string buffer in which the message is being constructed.
  *     If a safe buffer is specified, this function is
  *     thread safe.
  * @param errnum
  *     The error value to be decoded, usually obtain from the errno
  *     global variable just before this function is called.  This
  *     is necessary if you need to call <b>any</b> code between the
  *     system call to be explained and this function, because many libc
  *     functions will alter the value of errno.
  * @param oldpath
  *     The original oldpath, exactly has passed to the symlink(2) system call.
  * @param newpath
  *     The original newpath, exactly has passed to the symlink(2) system call.
  */
void explain_buffer_errno_symlink(struct explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_SYMLINK_H */
/* vim: set ts=8 sw=4 et : */
