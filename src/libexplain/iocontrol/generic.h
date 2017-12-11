/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_IOCONTROL_GENERIC_H
#define LIBEXPLAIN_IOCONTROL_GENERIC_H

#include <libexplain/iocontrol.h>

/**
  * The explain_iocontrol_generic global variable is
  * used to store information ablut the GENERIC
  * I/O control.
  *
  * @note
  *     This information is not kept in a single table for all values,
  *     like every other set of constants, because (a) some values
  *     are ambiguous, and (b) the includes files have bugs making it
  *     impossible to include all of them in the same combilation unit.
  */
extern const explain_iocontrol_t explain_iocontrol_generic;

/**
  * The explain_iocontrol_generic_print_explanation function is used to
  * print a generic (non-specific) explanation of an error return by an
  * ioctl system call.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_explanation(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_fake_syscall_name function is used to
  * synthesize a "sycall name" for use in error messages that take same.
  *
  * @param output
  *     where to write the name
  * @param output_size
  *     the available output size, in bytes
  * @param p
  *     ioctl descriptor
  * @param request
  *     original request, exactly as passed to the ioctl system call
  */
void explain_iocontrol_fake_syscall_name(char *output, int output_size,
    const explain_iocontrol_t *p, int request);

/**
  * The explain_iocontrol_generic_print_data_int function is used to
  * print a generic (non-specific) intger value arg (as opposed to some
  * kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_int(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_generic_print_data_uint function is used to
  * print a generic (non-specific) unsigned intger value arg (as opposed
  * to some kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_uint(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_generic_print_data_int_star function is used to
  * print a generic (non-specific) intger pointer arg (as opposed to any
  * other kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_int_star(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_generic_print_data_uint_star function is used
  * to print a generic (non-specific) unsigned intger pointer arg (as
  * opposed to any other kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_uint_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_long function is used to
  * print a generic (non-specific) long int value arg (as opposed to some
  * kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_long(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_generic_print_data_ulong function is used
  * to print a generic (non-specific) unsigned long int value arg (as
  * opposed to some kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_ulong(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_generic_print_data_long_star function is used
  * to print a generic (non-specific) intger pointer arg (as opposed to
  * any other kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_long_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_ulong_star function is used
  * to print a generic (non-specific) unsigned intger pointer arg (as
  * opposed to any other kind of pointer).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_ulong_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_pointer function is used
  * to print a generic pointer arg.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_pointer(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_int64_star function is used
  * to print a the signed 64-bit value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_int64_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_int64_star function is used
  * to print a the unsigned 64-bit value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_uint64_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_ignored function is used
  * to print a ioctl::data argument in the case where it is ignored (by
  * convention, should be zero).
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_ignored(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_short_star function is used
  * to print a the signed short value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_short_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_short_star function is used
  * to print a the unsigned short value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_ushort_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_int8_star function is used
  * to print a the signed int8 value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_int8_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_data_int8_star function is used
  * to print a the unsigned int8 value pointer to data.
  *
  * For use in iocontrol definitions, as code common to <i>many</i>
  * ioctl requests... hence the weird looking argument list.
  *
  * @param p
  *     ioctl descriptor
  * @param sb
  *     where to print the explanation
  * @param errnum
  *     the errno value to be explained
  * @param fildes
  *     original fildes, exactly as passed to the ioctl system call
  * @param request
  *     original request, exactly as passed to the ioctl system call
  * @param data
  *     original data, exactly as passed to the ioctl system call
  */
void explain_iocontrol_generic_print_data_uint8_star(
    const explain_iocontrol_t *p, struct explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data);

/**
  * The explain_iocontrol_generic_print_hash_define function is used to
  * turn an ioctl request number into the corresponding hash define, or what
  * it could have looked like had a hash define been used.
  *
  * @param sb
  *     The string buffer to print into.
  * @param request
  *     The ioctl request number to be printed.
  */
void explain_iocontrol_generic_print_hash_define(
    struct explain_string_buffer_t *sb, int request);

#endif /* LIBEXPLAIN_IOCONTROL_GENERIC_H */
/* vim: set ts=8 sw=4 et : */
