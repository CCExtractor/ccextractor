/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_TIME_H
#define LIBEXPLAIN_TIME_H

/**
  * @file
  * @brief explain time(2) errors
  *
  * These functions may be used to obtain explanations for errors returned
  * by the <i>time</i>(2) system call.
  */

#include <libexplain/gcc_attributes.h>

/*
 * We include <libexplain/large_file_support.h> because <time.h>, on
 * _some_ systems, winds up including <sys/types.h> and that may,
 * in turn, define off_t and other file-size related types.  So the
 * necessary preparation has to be done or things will break.
 */
#include <libexplain/large_file_support.h>

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_time_or_die function is used to call the <i>time</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the <i>#explain_time</i>(3) function, and then the
  * process terminates by calling <tt>exit(EXIT_FAILURE)</tt>.
  *
  * @param t
  *     The t, exactly as to be passed to the <i>time</i>(2) system call.
  * @returns
  *     This function only returns on success, see <i>time</i>(2) for more
  *     information. On failure, prints an explanation and exits, it does
  *     not return.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = explain_time_or_die(t);
  * @endcode
  */
time_t explain_time_or_die(time_t *t);

/**
  * The explain_time_on_error function is used to call the <i>time</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the <i>#explain_time</i>(3) function.
  *
  * @param t
  *     The t, exactly as to be passed to the <i>time</i>(2) system call.
  * @returns
  *     The value returned by the wrapped <i>time</i>(2) system call.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = explain_time_on_error(t);
  * if (result < 0)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  */
time_t explain_time_on_error(time_t *t)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_time function is used to obtain an explanation of an error
  * returned by the <i>time</i>(2) system call. The least the message will
  * contain is the value of <tt>strerror(errno)</tt>, but usually it will
  * do much better, and indicate the underlying cause in more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * @param t
  *     The original t, exactly as passed to the <i>time</i>(2) system
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
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = time(t);
  * if (result < 0)
  * {
  *     fprintf(stderr, "%s\n", explain_time(t));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_time_or_die function.
  */
const char *explain_time(time_t *t)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_time function is used to obtain an explanation of an
  * error returned by the <i>time</i>(2) system call. The least the message
  * will contain is the value of <tt>strerror(errnum)</tt>, but usually it
  * will do much better, and indicate the underlying cause in more detail.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param t
  *     The original t, exactly as passed to the <i>time</i>(2) system
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
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = time(t);
  * if (result < 0)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_time(err, t));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_time_or_die function.
  */
const char *explain_errno_time(int errnum, time_t *t)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_time function is used to obtain an explanation of
  * an error returned by the <i>time</i>(2) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param t
  *     The original t, exactly as passed to the <i>time</i>(2) system
  *     call.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = time(t);
  * if (result < 0)
  * {
  *     char message[3000];
  *     explain_message_time(message, sizeof(message), t);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_time_or_die function.
  */
void explain_message_time(char *message, int message_size, time_t *t);

/**
  * The explain_message_errno_time function is used to obtain an
  * explanation of an error returned by the <i>time</i>(2) system call. The
  * least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
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
  * @param t
  *     The original t, exactly as passed to the <i>time</i>(2) system
  *     call.
  *
   @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * time_t result = time(t);
  * if (result < 0)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_time(message, sizeof(message), err, t);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_time_or_die function.
  */
void explain_message_errno_time(char *message, int message_size, int errnum,
    time_t *t);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_TIME_H */
