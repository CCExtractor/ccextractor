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

#ifndef LIBEXPLAIN_LCHOWN_H
#define LIBEXPLAIN_LCHOWN_H

/**
  * @file
  * @brief explain lchown(2) errors
  */

#include <libexplain/gcc_attributes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_lchown_or_die function is used to call the <i>lchown</i>(2)
  * system call. On failure an explanation will be printed to stderr,
  * obtained from the explain_lchown(3) function, and then the process
  * terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * explain_lchown_or_die(pathname, owner, group);
  * @endcode
  *
  * @param pathname
  *     The pathname, exactly as to be passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The owner, exactly as to be passed to the <i>lchown</i>(2) system
  *     call.
  * @param group
  *     The group, exactly as to be passed to the <i>lchown</i>(2) system
  *     call.
  * @returns
  *     This function only returns on success. On failure, prints an
  *     explanation and exits, it does not return.
  */
void explain_lchown_or_die(const char *pathname, int owner, int group);

/**
  * The explain_lchown_on_error function is used to call the
  * <i>lchown</i>(2) system call. On failure an explanation will be printed
  * to stderr, obtained from the explain_lchown(3) function.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (explain_lchown_on_error(pathname, owner, group) < 0)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  *
  * @param pathname
  *     The pathname, exactly as to be passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The owner, exactly as to be passed to the <i>lchown</i>(2) system
  *     call.
  * @param group
  *     The group, exactly as to be passed to the <i>lchown</i>(2) system
  *     call.
  * @returns
  *     The value returned by the wrapped <i>lchown</i>(2) system call.
  */
int explain_lchown_on_error(const char *pathname, int owner, int group)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_lchown function is used to obtain an explanation of an
  * error returned by the <i>lchown</i>(2) system call. The least the
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
  * if (lchown(pathname, owner, group) < 0)
  * {
  *     fprintf(stderr, "%s\n", explain_lchown(pathname, owner, group));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_lchown_or_die function.
  *
  * @param pathname
  *     The original pathname, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The original owner, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param group
  *     The original group, exactly as passed to the <i>lchown</i>(2)
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
const char *explain_lchown(const char *pathname, int owner, int group)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_lchown function is used to obtain an explanation of
  * an error returned by the <i>lchown</i>(2) system call. The least the
  * message will contain is the value of <tt>strerror(errnum)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (lchown(pathname, owner, group) < 0)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_lchown(err, pathname, owner,
  *         group));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_lchown_or_die function.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param pathname
  *     The original pathname, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The original owner, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param group
  *     The original group, exactly as passed to the <i>lchown</i>(2)
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
const char *explain_errno_lchown(int errnum, const char *pathname, int owner,
    int group)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_lchown function is used to obtain an explanation of
  * an error returned by the <i>lchown</i>(2) system call. The least the
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
  * if (lchown(pathname, owner, group) < 0)
  * {
  *     char message[3000];
  *     explain_message_lchown(message, sizeof(message), pathname, owner,
  *         group);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_lchown_or_die function.
  *
  * @param message
  *     The location in which to store the returned message. If a suitable
  *     message return buffer is supplied, this function is thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param pathname
  *     The original pathname, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The original owner, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param group
  *     The original group, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  */
void explain_message_lchown(char *message, int message_size,
    const char *pathname, int owner, int group);

/**
  * The explain_message_errno_lchown function is used to obtain an
  * explanation of an error returned by the <i>lchown</i>(2) system call.
  * The least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (lchown(pathname, owner, group) < 0)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_lchown(message, sizeof(message), err, pathname,
  *         owner, group);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * The above code example is available pre-packaged as the
  * #explain_lchown_or_die function.
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
  * @param pathname
  *     The original pathname, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param owner
  *     The original owner, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  * @param group
  *     The original group, exactly as passed to the <i>lchown</i>(2)
  *     system call.
  */
void explain_message_errno_lchown(char *message, int message_size, int errnum,
    const char *pathname, int owner, int group);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_LCHOWN_H */
