/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_MALLOC_H
#define LIBEXPLAIN_MALLOC_H

/**
  * @file
  * @brief explain malloc(3) errors
  */

#include <libexplain/gcc_attributes.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_malloc_or_die function is used to call the <i>malloc</i>(3)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_malloc(3) function, and then the process
  * terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * void *result = explain_malloc_or_die(size);
  * @endcode
  *
  * @param size
  *     The size, exactly as to be passed to the <i>malloc</i>(3) system
  *     call.
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
void *explain_malloc_or_die(size_t size)
                                                     LIBEXPLAIN_ATTRIBUTE_MALLOC
                                             LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(1);

/**
  * The explain_malloc_on_error function is used to call the
  * <i>malloc</i>(3) system call. On failure an explanation will be printed
  * to stderr, obtained from the explain_malloc(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * void *result = explain_malloc_on_error(size);
  * if (!p)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @param size
  *     The size, exactly as to be passed to the <i>malloc</i>(3) system
  *     call.
  * @returns
  *     The value returned by the wrapped <i>malloc</i>(3) system call.
  */
void *explain_malloc_on_error(size_t size)
                                                     LIBEXPLAIN_ATTRIBUTE_MALLOC
                                              LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(1)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_malloc function is used to obtain an explanation of an
  * error returned by the <i>malloc</i>(3) system call. The least the
  * message will contain is the value of <tt>strerror(errno)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * void *result = malloc(size);
  * if (!result)
  * {
  *     fprintf(stderr, "%s\n", explain_malloc(size));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_malloc_or_die function.
  *
  * @param size
  *     The original size, exactly as passed to the <i>malloc</i>(3) system
  *     call.
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
const char *explain_malloc(size_t size)
                                                     LIBEXPLAIN_ATTRIBUTE_MALLOC
                                              LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(1)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_malloc function is used to obtain an explanation of
  * an error returned by the <i>malloc</i>(3) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * void *result = malloc(size);
  * if (!result)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_malloc(err, size));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_malloc_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param size
  *     The original size, exactly as passed to the <i>malloc</i>(3) system
  *     call.
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
const char *explain_errno_malloc(int errnum, size_t size)
                                              LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(2)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_malloc function is used to obtain an explanation of
  * an error returned by the <i>malloc</i>(3) system call. The least the
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
  * void *result = malloc(size);
  * if (!result)
  * {
  *     char message[3000];
  *     explain_message_malloc(message, sizeof(message), size);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_malloc_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param size
  *     The original size, exactly as passed to the <i>malloc</i>(3) system
  *     call.
  */
void explain_message_malloc(char *message, int message_size, size_t size)
                                             LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(3);

/**
  * The explain_message_errno_malloc function is used to obtain an
  * explanation of an error returned by the <i>malloc</i>(3) system call.
  * The least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * void *result = malloc(size);
  * if (!result)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_malloc(message, sizeof(message), err, size);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_malloc_or_die function.
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
  * @param size
  *     The original size, exactly as passed to the <i>malloc</i>(3) system
  *     call.
  */
void explain_message_errno_malloc(char *message, int message_size, int errnum,
    size_t size)
                                             LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(4);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_MALLOC_H */
