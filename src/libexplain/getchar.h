/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013, 2014 Peter Miller
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

#ifndef LIBEXPLAIN_GETCHAR_H
#define LIBEXPLAIN_GETCHAR_H

/**
  * @file
  * @brief explain getchar(3) errors
  */

#include <libexplain/gcc_attributes.h>
#include <libexplain/large_file_support.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FIXME: need a better inline-availability test,
 * but the defines in libexplain/config.h are not suitable for inclusion
 * in the client API because they have no LIBEXPLAIN_ namespace prefix.
 */
#if __GNUC__ >= 3 || defined(__clang__)
/**
 * Private function, provided for the exclusive use of the
 * explain_getchar_or_die inline function.  Clients of libexplain must
 * not use it, because it's existence and signature is subject to change
 * without notice.  Think of it as a C++ private method.
 */
void explain_getchar_or_die_failed(void);
#endif

/**
  * The explain_getchar_or_die function is used to call the
  * <i>getchar</i>(3) system call. On failure an explanation will be
  * printed to stderr, obtained from the explain_getchar(3) function, and
  * then the process terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int c = explain_getchar_or_die();
  * @endcode
  *
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
#if __GNUC__ >= 3 || defined(__clang__)
static __inline__
#endif
int explain_getchar_or_die(void)
#if __GNUC__ >= 3 || defined(__clang__)
__attribute__((always_inline))
#endif
;

#if __GNUC__ >= 3 || defined(__clang__)

static __inline__ int
explain_getchar_or_die(void)
{
    /*
     * By using inline, the users dosn't have to pay a one-function-
     * call-per-character penalty for using libexplain, because getc is
     * usually a macro or an inline that only calls underflow when the
     * buffer is exhausted.
     */
    int c = getchar();
    if (c == EOF && ferror(stdin))
        explain_getchar_or_die_failed();
    return c;
}

#endif

/**
  * The explain_getchar_on_error function is used to call the
  * <i>getchar</i>(3) system call. On failure an explanation will be
  * printed to stderr, obtained from the explain_getchar(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int c = explain_getchar_on_error();
  * if (c == EOF && ferror(stdin))
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @returns
  *     The value returned by the wrapped <i>getchar</i>(3) system call.
  */
int explain_getchar_on_error(void)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_getchar function is used to obtain an explanation of an
  * error returned by the <i>getchar</i>(3) system call. The least the
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
  * int c = getchar();
  * if (c == EOF && ferror(stdin))
  * {
  *     fprintf(stderr, "%s\n", explain_getchar());
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_getchar_or_die function.
  *
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
const char *explain_getchar(void)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_getchar function is used to obtain an explanation of
  * an error returned by the <i>getchar</i>(3) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int c = getchar();
  * if (c == EOF && ferror(stdin))
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_getchar(err, ));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_getchar_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
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
const char *explain_errno_getchar(int errnum)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_getchar function is used to obtain an explanation
  * of an error returned by the <i>getchar</i>(3) system call. The least
  * the message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int c = getchar();
  * if (c == EOF && ferror(stdin))
  * {
  *     char message[3000];
  *     explain_message_getchar(message, sizeof(message), );
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_getchar_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  */
void explain_message_getchar(char *message, int message_size);

/**
  * The explain_message_errno_getchar function is used to obtain an
  * explanation of an error returned by the <i>getchar</i>(3) system call.
  * The least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int c = getchar();
  * if (c == EOF && ferror(stdin))
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_getchar(message, sizeof(message), err, );
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_getchar_or_die function.
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
  */
void explain_message_errno_getchar(char *message, int message_size, int errnum);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_GETCHAR_H */
