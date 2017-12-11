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

#ifndef LIBEXPLAIN_SELECT_H
#define LIBEXPLAIN_SELECT_H

/**
  * @file
  * @brief explain select(2) errors
  */

#include <libexplain/gcc_attributes.h>
#include <libexplain/large_file_support.h>

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_select_or_die function is used to call the <i>select</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_select(3) function, and then the process
  * terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * explain_select_or_die(nfds, readfds, writefds, exceptfds, timeout);
  * @endcode
  *
  * @param nfds
  *     The nfds, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The readfds, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @param writefds
  *     The writefds, exactly as to be passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The exceptfds, exactly as to be passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The timeout, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
int explain_select_or_die(int nfds, fd_set *readfds, fd_set *writefds,
    fd_set *exceptfds, struct timeval *timeout);

/**
  * The explain_select_on_error function is used to call the
  * <i>select</i>(2) system call. On failure an explanation will be printed
  * to stderr, obtained from the explain_select(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (explain_select_on_error(nfds, readfds, writefds, exceptfds, timeout)
  *     < 0)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @param nfds
  *     The nfds, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The readfds, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @param writefds
  *     The writefds, exactly as to be passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The exceptfds, exactly as to be passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The timeout, exactly as to be passed to the <i>select</i>(2) system
  *     call.
  * @returns
  *     The value returned by the wrapped <i>select</i>(2) system call.
  */
int explain_select_on_error(int nfds, fd_set *readfds, fd_set *writefds,
    fd_set *exceptfds, struct timeval *timeout)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_select function is used to obtain an explanation of an
  * error returned by the <i>select</i>(2) system call. The least the
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
  * if (select(nfds, readfds, writefds, exceptfds, timeout) < 0)
  * {
  *     fprintf(stderr, "%s\n", explain_select(nfds, readfds, writefds,
  *         exceptfds, timeout));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_select_or_die function.
  *
  * @param nfds
  *     The original nfds, exactly as passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The original readfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param writefds
  *     The original writefds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The original exceptfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The original timeout, exactly as passed to the <i>select</i>(2)
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
const char *explain_select(int nfds, fd_set *readfds, fd_set *writefds,
    fd_set *exceptfds, struct timeval *timeout)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_select function is used to obtain an explanation of
  * an error returned by the <i>select</i>(2) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (select(nfds, readfds, writefds, exceptfds, timeout) < 0)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_select(err, nfds, readfds,
  *         writefds, exceptfds, timeout));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_select_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param nfds
  *     The original nfds, exactly as passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The original readfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param writefds
  *     The original writefds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The original exceptfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The original timeout, exactly as passed to the <i>select</i>(2)
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
const char *explain_errno_select(int errnum, int nfds, fd_set *readfds,
    fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_select function is used to obtain an explanation of
  * an error returned by the <i>select</i>(2) system call. The least the
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
  * if (select(nfds, readfds, writefds, exceptfds, timeout) < 0)
  * {
  *     char message[3000];
  *     explain_message_select(message, sizeof(message), nfds, readfds,
  *         writefds, exceptfds, timeout);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_select_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param nfds
  *     The original nfds, exactly as passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The original readfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param writefds
  *     The original writefds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The original exceptfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The original timeout, exactly as passed to the <i>select</i>(2)
  *     system call.
  */
void explain_message_select(char *message, int message_size, int nfds,
    fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout);

/**
  * The explain_message_errno_select function is used to obtain an
  * explanation of an error returned by the <i>select</i>(2) system call.
  * The least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (select(nfds, readfds, writefds, exceptfds, timeout) < 0)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_select(message, sizeof(message), err, nfds,
  *         readfds, writefds, exceptfds, timeout);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_select_or_die function.
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
  * @param nfds
  *     The original nfds, exactly as passed to the <i>select</i>(2) system
  *     call.
  * @param readfds
  *     The original readfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param writefds
  *     The original writefds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param exceptfds
  *     The original exceptfds, exactly as passed to the <i>select</i>(2)
  *     system call.
  * @param timeout
  *     The original timeout, exactly as passed to the <i>select</i>(2)
  *     system call.
  */
void explain_message_errno_select(char *message, int message_size, int errnum,
    int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_SELECT_H */
