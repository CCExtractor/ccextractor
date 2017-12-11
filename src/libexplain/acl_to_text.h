/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_ACL_TO_TEXT_H
#define LIBEXPLAIN_ACL_TO_TEXT_H

/**
  * @file
  * @brief explain acl_to_text(3) errors
  *
  * These functions may be used to obtain explanations for errors returned
  * by the <i>acl_to_text</i>(3) system call.
  */

#include <libexplain/gcc_attributes.h>
#include <libexplain/large_file_support.h>

#include <sys/acl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_acl_to_text_or_die function is used to call the
  * <i>acl_to_text</i>(3) system call. On failure an explanation will be
  * printed to stderr, obtained from the <i>#explain_acl_to_text</i>(3)
  * function, and then the process terminates by calling
  * <tt>exit(EXIT_FAILURE)</tt>.
  *
  * @param acl
  *     The acl, exactly as to be passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The len_p, exactly as to be passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @returns
  *     This function only returns on success, see <i>acl_to_text</i>(3)
  *     for more information. On failure, prints an explanation and exits,
  *     it does not return.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = explain_acl_to_text_or_die(acl, len_p);
  * @endcode
  */
char *explain_acl_to_text_or_die(acl_t acl, ssize_t *len_p)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_acl_to_text_on_error function is used to call the
  * <i>acl_to_text</i>(3) system call. On failure an explanation will be
  * printed to stderr, obtained from the <i>#explain_acl_to_text</i>(3)
  * function.
  *
  * @param acl
  *     The acl, exactly as to be passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The len_p, exactly as to be passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @returns
  *     The value returned by the wrapped <i>acl_to_text</i>(3) system
  *     call.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = explain_acl_to_text_on_error(acl, len_p);
  * if (!result)
  * {
  *     ...cope with error
  *     ...no need to print error message
  * }
  * @endcode
  */
char *explain_acl_to_text_on_error(acl_t acl, ssize_t *len_p)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_acl_to_text function is used to obtain an explanation of an
  * error returned by the <i>acl_to_text</i>(3) system call. The least the
  * message will contain is the value of <tt>strerror(errno)</tt>, but
  * usually it will do much better, and indicate the underlying cause in
  * more detail.
  *
  * The errno global variable will be used to obtain the error value to be
  * decoded.
  *
  * @param acl
  *     The original acl, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The original len_p, exactly as passed to the <i>acl_to_text</i>(3)
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
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = acl_to_text(acl, len_p);
  * if (!result)
  * {
  *     explain_output_error_and_die("%s\n", explain_acl_to_text(acl, len_p));
  * }
  * @endcode
  * @par
  * The above code example is available pre\-packaged as the
  * #explain_acl_to_text_or_die function.
  */
const char *explain_acl_to_text(acl_t acl, ssize_t *len_p)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_errno_acl_to_text function is used to obtain an explanation
  * of an error returned by the <i>acl_to_text</i>(3) system call. The
  * least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * @param errnum
  *     The error value to be decoded, usually obtained from the errno
  *     global variable just before this function is called. This is
  *     necessary if you need to call <b>any</b> code between the system
  *     call to be explained and this function, because many libc functions
  *     will alter the value of errno.
  * @param acl
  *     The original acl, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The original len_p, exactly as passed to the <i>acl_to_text</i>(3)
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
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = acl_to_text(acl, len_p);
  * if (!result)
  * {
  *     int err = errno;
  *     fprintf(stderr, "%s\n", explain_errno_acl_to_text(err, acl, len_p));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_acl_to_text_or_die function.
  */
const char *explain_errno_acl_to_text(int errnum, acl_t acl, ssize_t *len_p)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_acl_to_text function is used to obtain an
  * explanation of an error returned by the <i>acl_to_text</i>(3) system
  * call. The least the message will contain is the value of
  * <tt>strerror(errnum)</tt>, but usually it will do much better, and
  * indicate the underlying cause in more detail.
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
  * @param acl
  *     The original acl, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The original len_p, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = acl_to_text(acl, len_p);
  * if (!result)
  * {
  *     char message[3000];
  *     explain_message_acl_to_text(message, sizeof(message), acl, len_p);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_acl_to_text_or_die function.
  */
void explain_message_acl_to_text(char *message, int message_size, acl_t acl,
    ssize_t *len_p);

/**
  * The explain_message_errno_acl_to_text function is used to obtain an
  * explanation of an error returned by the <i>acl_to_text</i>(3) system
  * call. The least the message will contain is the value of
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
  * @param acl
  *     The original acl, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  * @param len_p
  *     The original len_p, exactly as passed to the <i>acl_to_text</i>(3)
  *     system call.
  *
  * @par Example:
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * char *result = acl_to_text(acl, len_p);
  * if (!result)
  * {
  *     int err = errno;
  *     char message[3000];
  *     explain_message_errno_acl_to_text(message, sizeof(message), err, acl,
  *         len_p);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  * @par
  * The above code example is available pre-packaged as the
  * #explain_acl_to_text_or_die function.
  */
void explain_message_errno_acl_to_text(char *message, int message_size, int
    errnum, acl_t acl, ssize_t *len_p);

#ifdef __cplusplus
}
#endif

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_ACL_TO_TEXT_H */
