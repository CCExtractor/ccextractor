/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_BUFFER_EINVAL_H
#define LIBEXPLAIN_BUFFER_EINVAL_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_einval_bits function may be used to
  * explain a bit-field containing undefined bits.
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  */
void explain_buffer_einval_bits(explain_string_buffer_t *sb,
    const char *caption);

/**
  * The explain_buffer_einval_too_small function may be used to
  * explain that a system call argument is too small (out of range).
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  * @param data
  *    The value of the offending system call argument.
  */
void explain_buffer_einval_too_small(explain_string_buffer_t *sb,
    const char *caption, long data);

/**
  * The explain_buffer_einval_too_large function may be used to
  * explain that a system call argument is too large (out of range).
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  */
void explain_buffer_einval_too_large(explain_string_buffer_t *sb,
    const char *caption);

/**
  * The explain_buffer_einval_too_large2 function may be used to
  * explain that a system call argument is too large (out of range).
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  * @param max
  *    The actual upper limit.
  */
void explain_buffer_einval_too_large2(explain_string_buffer_t *sb,
    const char *caption, long max);

/**
  * The explain_buffer_einval_vague function may be used to explain
  * that a system call argument is invalid, but not why (use this one
  * sparingly).
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  */
void explain_buffer_einval_vague(explain_string_buffer_t *sb,
    const char *caption);

/**
  * The explain_buffer_einval_value function may be used to explain
  * that a system call argument is invalid, but not why.
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  * @param value
  *    The value of the offending system call argument.
  */
void explain_buffer_einval_value(explain_string_buffer_t *sb,
    const char *caption, long value);

/**
  * The explain_buffer_einval_not_a_number function may be used to
  * explain that a system call argument is invalid, when it is supposed
  * to be a string containing a number.
  *
  * @param sb
  *    The string buffer to print into.
  * @param caption
  *    The name of the offending system call argument.
  */
void explain_buffer_einval_not_a_number(explain_string_buffer_t *sb,
    const char *caption);

void explain_buffer_einval_not_listening(explain_string_buffer_t *sb);

/**
  * The explain_buffer_einval_out_of_range functions is used to print
  * an explanation for an EINVAL error, in the case where an argument's
  * value is outside the valid range.
  *
  * @param sb
  *     The buffer to print into.
  * @param caption
  *     The name of the offending system call argument.
  * @param lo
  *     The lower bound of the valid range.
  * @param hi
  *     The upper bound of the valid range.
  */
void explain_buffer_einval_out_of_range(explain_string_buffer_t *sb,
    const char *caption, long lo, long hi);

/**
  * The explain_buffer_einval_signalfd function is use to print an
  * explanation of an EINVAL error reported by the signalfd system
  * call reposrt an EINVAL error, in the case where the file descriptor
  * is actually open, but does not refer to a valid signalfd file
  * descriptor.
  *
  * @param sb
  *     The buffer to print into.
  * @param caption
  *     The name of the offending system call argument.
  */
void explain_buffer_einval_signalfd(explain_string_buffer_t *sb,
    const char *caption);

/**
  * The explain_buffer_einval_multiple function is use to print an
  * explanation of an EINVAL error, in the case where an argument is
  * meant to be a multiple of a number (e.g. block sizes).
  *
  * @param sb
  *     The buffer to print into.
  * @param caption
  *     The name of the offending system call argument.
  * @param multiple
  *     The number it must be a multiple of.
  */
void explain_buffer_einval_multiple(explain_string_buffer_t *sb,
    const char *caption, int multiple);

/**
  * The explain_buffer_einval_multiple function is use to print an
  * explanation of an EINVAL error, when reported by mknod.
  *
  * @param sb
  *     The buffer to print into.
  * @param mode
  *     The kind of node the syscall was trying to create.
  * @param mode_caption
  *     The name of the offending system call argument.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_einval_mknod(explain_string_buffer_t *sb, int mode,
    const char *mode_caption, const char *syscall_name);

/**
  * The explain_buffer_einval_mkstemp function is use to print an
  * explanation of an EINVAL error, when reported by mknod.
  *
  * @param sb
  *     The buffer to print into.
  * @param templat
  *     The offending file name template.
  * @param templat_caption
  *     The offending syscall argument.
  */
void explain_buffer_einval_mkstemp(explain_string_buffer_t *sb,
    const char *templat, const char *templat_caption);

/**
  * The explain_buffer_einval_setenv function is use to print an
  * explanation of an EINVAL error, when reported by setenv.
  *
  * @param sb
  *     The buffer to print into.
  * @param name
  *     The environment variable name.
  * @param name_caption
  *     The name of the syscall argument containing the environment
  *     variable name.
  */
void explain_buffer_einval_setenv(explain_string_buffer_t *sb, const char *name,
    const char *name_caption);

struct sock_fprog;

/**
  * The explain_buffer_einval_sock_fprog function is use to print an
  * explanation of an EINVAL error, when reported by PPP ioctls.
  *
  * @param sb
  *     The buffer to print into.
  * @param data
  *     The filter with the problem.
  */
void explain_buffer_einval_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data);

/**
  * The explain_buffer_einval_ungetc function is used to report problems
  * trying to push back EOF onto streams.
  *
  * @param sb
  *     The buffer to print into.
  * @param syscall_name
  *     The name of the offended system call.
  */
void explain_buffer_einval_ungetc(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_einval_format_string function is used to report
  * problems concerning invalid format strings.
  *
  * @param sb
  *     The buffer to print into.
  * @param argument_name
  *     The name of the offending system call argument.
  * @param argument_value
  *     The value of the offending system call argument (format string).
  * @param errpos
  *     The position within the string that the error was discovered.
  */
void explain_buffer_einval_format_string(explain_string_buffer_t *sb,
    const char *argument_name, const char *argument_value, size_t errpos);

/**
  * The explain_buffer_einval_format_string_hole function is used to report
  * problems concerning invalid format strings, in the case where %n$
  * does not appear in the format string when it should.
  *
  * @param sb
  *     The buffer to print into.
  * @param argument_name
  *     The name of the offending system call argument.
  * @param hole_index
  *     The %n$ argument that has not been used.
  */
void explain_buffer_einval_format_string_hole(explain_string_buffer_t *sb,
    const char *argument_name, int hole_index);

/**
  * The explain_buffer_einval_no_vid_std function is used to report
  * problems with a V4L2 ioctl, in the case where no output video
  * standards are supported.
  *
  * @param sb
  *     The buffer to print into.
  */
void explain_buffer_einval_no_vid_std(explain_string_buffer_t *sb);

#endif /* LIBEXPLAIN_BUFFER_EINVAL_H */
/* vim: set ts=8 sw=4 et : */
