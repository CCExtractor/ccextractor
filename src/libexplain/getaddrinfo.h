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

#ifndef LIBEXPLAIN_GETADDRINFO_H
#define LIBEXPLAIN_GETADDRINFO_H

/**
  * @file
  * @brief explain getaddrinfo(3) errors
  */

#include <libexplain/gcc_attributes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct addrinfo; /* forward */

/**
  * The explain_getaddrinfo_or_die function is used to call the
  * getaddrinfo(3) system call.  On failure an explanation will be
  * printed to stderr, obtained from explain_errcode_getaddrinfo(3),
  * and then the process terminates by calling exit(EXIT_FAILURE).
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * explain_getaddrinfo_or_die(node, service, hints, res);
  * @endcode
  *
  * @param node
  *     The node, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @param service
  *     The service, exactly as to be passed to the getaddrinfo(3)
  *     system call.
  * @param hints
  *     The hints, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @param res
  *     The res, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @returns
  *     This function only returns on success.  On failure, prints an
  *     explanation and exits, it does not return.
  */
void explain_getaddrinfo_or_die(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res);

/**
  * The explain_getaddrinfo_on_error function is used to call the
  * getaddrinfo(3) system call.  On failure an explanation will be
  * printed to stderr, but it still returns.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * if (explain_getaddrinfo_on_error(node, service, hints, res))
  * {
  *     ...handle error
  *     ...error message already printed
  * }
  * @endcode
  *
  * @param node
  *     The node, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @param service
  *     The service, exactly as to be passed to the getaddrinfo(3)
  *     system call.
  * @param hints
  *     The hints, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @param res
  *     The res, exactly as to be passed to the getaddrinfo(3) system
  *     call.
  * @returns
  *     This function only returns on success.  On failure, prints an
  *     explanation and exits, it does not return.
  */
int explain_getaddrinfo_on_error(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res);

/**
  * The explain_errcode_getaddrinfo function is used to obtain an
  * explanation of an error returned by the getaddrinfo(3) system
  * call.  The least the message will contain is the value of
  * gai_strerror(errcode), but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion
  * similar to the following example:
  * @code
  * int errcode = getaddrinfo(node, service, hints, res);
  * if (errcode == EAI_SYSTEM)
  *     errcode = errno;
  * if (errcode)
  * {
  *     fprintf(stderr, "%s\n", explain_errcode_getaddrinfo(errcode,
  *         node, service, hints, res));
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * @param errnum
  *     The error value to be decoded, as returned by the getaddrinfo(3)
  *     system call.
  * @param node
  *     The original node, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param service
  *     The original service, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param hints
  *     The original hints, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param res
  *     The original res, exactly as passed to the getaddrinfo(3) system
  *     call.
  * @returns
  *     The message explaining the error.  This message buffer is shared
  *     by all libexplain functions which do not supply a buffer in
  *     their argument list.  This will be overwritten by the next call
  *     to any libexplain function which shares this buffer, including
  *     other threads.
  * @note
  *     This function is <b>not</b> thread safe, because it shares a
  *     return buffer across all threads, and many other functions in
  *     this library.
  */
const char *explain_errcode_getaddrinfo(int errnum, const char *node,
    const char *service, const struct addrinfo *hints, struct addrinfo **res)
                                                  LIBEXPLAIN_WARN_UNUSED_RESULT;

/**
  * The explain_message_errcode_getaddrinfo function is used to
  * obtain an explanation of an error returned by the getaddrinfo(3)
  * system call.  The least the message will contain is the value of
  * gai_strerror(errcode), but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * This function is intended to be used in a fashion similar to the
  * following example:
  * @code
  * int errcode = getaddrinfo(node, service, hints, res);
  * if (errcode == EAI_SYSTEM)
  *     errcode = errno;
  * if (errcode)
  * {
  *     char message[3000];
  *     explain_message_errcode_getaddrinfo(message, sizeof(message),
  *         errcode, node, service, hints, res);
  *     fprintf(stderr, "%s\n", message);
  *     exit(EXIT_FAILURE);
  * }
  * @endcode
  *
  * @param message
  *     The location in which to store the returned message.  If a
  *     suitable message return buffer is supplied, this function is
  *     thread safe.
  * @param message_size
  *     The size in bytes of the location in which to store the returned
  *     message.
  * @param errcode
  *     The error value to be decoded, as returned by the getaddrinfo
  *     system call.
  * @param node
  *     The original node, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param service
  *     The original service, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param hints
  *     The original hints, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param res
  *     The original res, exactly as passed to the getaddrinfo(3) system
  *     call.
  */
void explain_message_errcode_getaddrinfo(char *message, int message_size,
    int errcode, const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res);

#ifdef __cplusplus
}
#endif

#endif /* LIBEXPLAIN_GETADDRINFO_H */
/* vim: set ts=8 sw=4 et : */
