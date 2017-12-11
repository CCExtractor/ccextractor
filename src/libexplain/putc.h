/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_PUTC_H
#define LIBEXPLAIN_PUTC_H

/**
  * @file
  * @brief explain putc(3) errors
  */

#include <libexplain/gcc_attributes.h>
#include <libexplain/gcc_attributes.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * Private function, provided for the explusive use of the
  * explain_putc_or_die inline function.  Clients of libexplain must
  * not use it, because it's existence and signature is subject to
  * change without notice.  Think of it as a C++ private method.
  */
void explain_putc_or_die_failed(int c, FILE *fp);

/**
  * Private function, provided for the explusive use of the
  * explain_putc_on_error inline function.  Clients of libexplain must
  * not use it, because it's existence and signature is subject to
  * change without notice.  Think of it as a C++ private method.
  */
void explain_putc_on_error_failed(int c, FILE *fp);

/**
  * The explain_putc_or_die function is used to call the <i>putc</i>(3)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_putc(3) function, and then the process
  * terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * explain_putc_or_die(c, fp);
  * @endcode
  *
  * @param c
  *     The c, exactly as to be passed to the <i>putc</i>(3) system call.
  * @param fp
  *     The fp, exactly as to be passed to the <i>putc</i>(3) system call.
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
#if __GNUC__ >= 3
static __inline__
#endif
void explain_putc_or_die(int c, FILE *fp)
#if __GNUC__ >= 3
__attribute__((always_inline))
#endif
    ;

#if __GNUC__ >= 3

static __inline__ void
explain_putc_or_die(int c, FILE *fp)
{
    /*
     * By using inline, the user doesn't have to pay a one-function-
     * call-per-character penalty for using libexplain, because putc is
     * usually a macro or an inline that only calls overflow when the
     * buffer is exhausted.
     */
    if (putc(c, fp) == EOF)
        explain_putc_or_die_failed(c, fp);
}

#endif

/**
  * The explain_putc_on_error function is used to call the <i>putc</i>(3)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_putc(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (explain_putc_on_error(c, fp) == EOF)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @param c
  *     The c, exactly as to be passed to the <i>putc</i>(3) system call.
  * @param fp
  *     The fp, exactly as to be passed to the <i>putc</i>(3) system call.
  * @returns
  *     The value returned by the wrapped <i>putc</i>(3) system call.
  */
#if __GNUC__ >= 3
static __inline__
#endif
int explain_putc_on_error(int c, FILE *fp)
#if __GNUC__ >= 3
__attribute__((always_inline))
#endif
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

#if __GNUC__ >= 3

static __inline__ int
explain_putc_on_error(int c, FILE *fp)
{
    /*
     * By using inline, the user doesn't have to pay a one-function-
     * call-per-character penalty for using libexplain, because putc is
     * usually a macro or an inline that only calls overflow when the
     * buffer is exhausted.
     */
    int result = putc(c, fp);
    if (result == EOF)
        explain_putc_on_error_failed(c, fp);
    return result;
}

#endif

/**
  * The explain_putc function is used to obtain an explanation of an error
  * returned by the <i>putc</i>(3) system call. The least the message will
  * contain is the value of <tt>strerror(errno)</tt>, but usually it will
  * do much better, and indicate the underlying cause in more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (putc(c, fp) == EOF)
  * {
  *     fprintf(stderr, "%s\n", explain_putc(c, fp));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_putc_or_die function.
  *
  * @param c
  *     The original c, exactly as passed to the <i>putc</i>(3) system
  *     call.
  * @param fp
  *     The original fp, exactly as passed to the <i>putc</i>(3) system
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
const char *explain_putc(int c, FILE *fp)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_putc function is used to obtain an explanation of an
  * error returned by the <i>putc</i>(3) system call. The least the message
  * will contain is the value of <tt>strerror(errnum)</tt>, but usually it
  * will do much better, and indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (putc(c, fp) == EOF)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_putc(err, c, fp));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_putc_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param c
  *     The original c, exactly as passed to the <i>putc</i>(3) system
  *     call.
  * @param fp
  *     The original fp, exactly as passed to the <i>putc</i>(3) system
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
const char *explain_errno_putc(int errnum, int c, FILE *fp)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_putc function is used to obtain an explanation of
  * an error returned by the <i>putc</i>(3) system call. The least the
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
  * if (putc(c, fp) == EOF)
  * {
  *     char message[3000];
  *     explain_message_putc(message, sizeof(message), c, fp);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_putc_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param c
  *     The original c, exactly as passed to the <i>putc</i>(3) system
  *     call.
  * @param fp
  *     The original fp, exactly as passed to the <i>putc</i>(3) system
  *     call.
  */
void explain_message_putc(char *message, int message_size, int c, FILE *fp);

/**
  * The explain_message_errno_putc function is used to obtain an
  * explanation of an error returned by the <i>putc</i>(3) system call. The
  * least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (putc(c, fp) == EOF)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_putc(message, sizeof(message), err, c, fp);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_putc_or_die function.
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
  * @param c
  *     The original c, exactly as passed to the <i>putc</i>(3) system
  *     call.
  * @param fp
  *     The original fp, exactly as passed to the <i>putc</i>(3) system
  *     call.
  */
void explain_message_errno_putc(char *message, int message_size, int errnum,
    int c, FILE *fp);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_PUTC_H */
