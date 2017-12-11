/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_READ_H
#define LIBEXPLAIN_READ_H

/**
  * @file
  * @brief explain read(2) errors
  */

#include <libexplain/gcc_attributes.h>
#include <libexplain/large_file_support.h>

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_read_or_die function is used to call the <i>read</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_read(3) function, and then the process
  * terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = explain_read_or_die(fildes, data, data_size);
  * @endcode
  *
  * @param fildes
  *     The fildes, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The data, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The data_size, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
ssize_t explain_read_or_die(int fildes, void *data, size_t data_size);

/**
  * The explain_read_on_error function is used to call the <i>read</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_read(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = explain_read_on_error(fildes, data, data_size);
  * if (result < 0)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @param fildes
  *     The fildes, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The data, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The data_size, exactly as to be passed to the <i>read</i>(2) system
  *     call.
  * @returns
  *     The value returned by the wrapped <i>read</i>(2) system call.
  */
ssize_t explain_read_on_error(int fildes, void *data, size_t data_size)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_read function is used to obtain an explanation of an error
  * returned by the <i>read</i>(2) system call. The least the message will
  * contain is the value of <tt>strerror(errno)</tt>, but usually it will
  * do much better, and indicate the underlying cause in more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = read(fildes, data, data_size);
  * if (result < 0)
  * {
  *     fprintf(stderr, "%s\n", explain_read(fildes, data, data_size));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_read_or_die function.
  *
  * @param fildes
  *     The original fildes, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The original data, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The original data_size, exactly as passed to the <i>read</i>(2)
  *     system call.
  * @returns
  *     The message explaining the error. This message buffer is shared by
  *     all libexplain functions which do not supply a buffer in their
  *     argument list. This will be overwritten by the next call to any
  *     libexplain function which shares this buffer, including other
  *     threads.
  * @note
  *     This function is <b>not</b> thread safe, because it shares a return
  *     buffer across all threads, and many other functions in this
  *     library.
  */
const char *explain_read(int fildes, const void *data, size_t data_size)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_read function is used to obtain an explanation of an
  * error returned by the <i>read</i>(2) system call. The least the message
  * will contain is the value of <tt>strerror(errnum)</tt>, but usually it
  * will do much better, and indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = read(fildes, data, data_size);
  * if (result < 0)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_read(err, fildes, data,
  *         data_size));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_read_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param fildes
  *     The original fildes, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The original data, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The original data_size, exactly as passed to the <i>read</i>(2)
  *     system call.
  * @returns
  *     The message explaining the error. This message buffer is shared by
  *     all libexplain functions which do not supply a buffer in their
  *     argument list. This will be overwritten by the next call to any
  *     libexplain function which shares this buffer, including other
  *     threads.
  * @note
  *     This function is <b>not</b> thread safe, because it shares a return
  *     buffer across all threads, and many other functions in this
  *     library.
  */
const char *explain_errno_read(int errnum, int fildes, const void *data,
    size_t data_size)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_read function is used to obtain an explanation of
  * an error returned by the <i>read</i>(2) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = read(fildes, data, data_size);
  * if (result < 0)
  * {
  *     char message[3000];
  *     explain_message_read(message, sizeof(message), fildes, data, data_size);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_read_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param fildes
  *     The original fildes, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The original data, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The original data_size, exactly as passed to the <i>read</i>(2)
  *     system call.
  */
void explain_message_read(char *message, int message_size, int fildes,
    const void *data, size_t data_size);

/**
  * The explain_message_errno_read function is used to obtain an
  * explanation of an error returned by the <i>read</i>(2) system call. The
  * least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * ssize_t result = read(fildes, data, data_size);
  * if (result < 0)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_read(message, sizeof(message), err, fildes, data,
  *         data_size);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_read_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param fildes
  *     The original fildes, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data
  *     The original data, exactly as passed to the <i>read</i>(2) system
  *     call.
  * @param data_size
  *     The original data_size, exactly as passed to the <i>read</i>(2)
  *     system call.
  */
void explain_message_errno_read(char *message, int message_size, int errnum,
    int fildes, const void *data, size_t data_size);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_READ_H */
