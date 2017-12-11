/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_IOCONTROL_H
#define LIBEXPLAIN_IOCONTROL_H

struct explain_string_buffer_t; /* forward */
typedef struct explain_iocontrol_t explain_iocontrol_t;

/**
  * The expain_iocontrol_print_func_t type is used to represent a
  * pointer to a function that prints somethign related to an ioctl
  * request.
  *
  * @param p
  *     Pointer to the explain_iocontrol_t instance,
  *     rather like "this" in C++ code.
  * @param sb
  *     The string buffer to print into.
  * @param errnum
  *     The error number that caused all this, obtained from the
  *     global errno variable, possibly indirectly.
  * @param fildes
  *     The original fildes, exactly as passed to the ioctl(2) system call.
  * @param request
  *     The original request, exactly as passed to the ioctl(2) system call.
  * @param data
  *     The original data, exactly as passed to the ioctl(2) system call.
  */
typedef void (*expain_iocontrol_print_func_t)(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_t type is used to represent an ioctl(2) call,
  * providing name and number.
  *
  * @note
  *     This information is not kept in a single table for all values,
  *     like every other set of constants, because (a) some values
  *     are ambiguous, and (b) the includes files have bugs making it
  *     impossible to include all of them in the same combilation unit.
  */
struct explain_iocontrol_t
{
    /**
      * The name of the ioctl(2) request.
      *
      * Set to NULL for systems on which this ioctl does not apply.
      */
    const char *name;

    /**
      * The numeric value of the ioctl(2) request.
      *
      * Set to zero (0) for systems on which this ioctl does not apply.
      */
    int number;

    /**
      * The disambiguate method is used to ambiguous ioctl(2) request
      * values, to attempt to discern which was intended from
      * information about the state of the file descriptor, and the
      * request itself.  This is not always possible, given that the
      * code is not psychic.
      *
      * If the NULL pointer value is used, the value is assumed to be
      * non-ambiguous.  This is almost always the case.
      *
      * Returns 0 on success (i.e. a match), -1 on failure (i.e. not a match).
      */
    int (*disambiguate)(int fildes, int request, const void *data);

    /**
      * The print_name member is a pointer to a function that is is used
      * to print the name of a request argument passed to an ioctl(2)
      * system call.
      *
      * If the NULL pointer is used, the default action is to print the
      * name member.
      */
    expain_iocontrol_print_func_t print_name;

    /**
      * The print_data member is a pointer to a function that is used
      * to print a representation of the data argument passed to an
      * ioctl(2) system call.
      */
    expain_iocontrol_print_func_t print_data;

    /**
      * The print_explanation member is a pointer to a function that
      * is used to print an representation of the data returned by
      * a successful ioctl(2) system call.
      */
    expain_iocontrol_print_func_t print_explanation;

    /**
      * The print_data_returned method is used to print a representation
      * of the data argument returned by an ioctl(2) system call.
      *
      * @param p
      *     Pointer to the explain_iocontrol_t instance,
      *     rather like "this" in C++ code.
      * @param sb
      *     The string buffer to print into.
      * @param errnum
      *     The error number that caused all this, obtained from the
      *     global errno variable, possibly indirectly.
      * @param fildes
      *     The original fildes, exactly as passed to the ioctl(2) system call.
      * @param request
      *     The original request, exactly as passed to the ioctl(2) system call.
      * @param data
      *     The original data, exactly as passed to the ioctl(2) system call.
      */
    void (*print_data_returned)(const explain_iocontrol_t *p,
        struct explain_string_buffer_t *sb, int errnum, int fildes,
        int request, const void *data);

    /**
      * The data_size member is used to remember the sizeof the data
      * pointed to by the ioctl data argument.  This is used by ioctl
      * scan to allocate data of the correct size.
      *
      * If set to NOT_A_POINTER, it means that the data passed to the ioctl
      * is not a pointer (an intptr_t instead).
      */
    unsigned data_size;

    /**
      * The data_type member is used to remember the type of the data
      * pointed to by the ioctl data argument.  This is used to improve the
      * description, but also to provide this to static analysis tools.
      */
    const char *data_type;

    /**
      * The flags member is used to store assorted information about the
      * ioctl, mostly of interest when checking ioctl data for internal
      * consistency.
      */
    unsigned flags;

    /**
      * The file member is used to rememebr the name of the file this
      * ioctl is defined in, by using __FILE__ in the initialiser.
      * This is needed to obtain useful error message out of the
      * explain_iocontrol_check_conflicts function.
      */
    const char *file;

    /**
      * The line member is used to rememebr the line number of the
      * source file this ioctl is defined in, by using __LINE__ in the
      * initialiser.  This is needed to obtain useful error message out
      * of the explain_iocontrol_check_conflicts function.
      */
    int line;
};

/**
  * The explain_iocontrol_find_by_number function may be used to
  * locate an ioctl by number.  A few ioctl(2) calls are ambiguous, so
  * the more information you can give the better.
  *
  * @param fildes
  *     The file descriptor the ioctl(2) call is made against
  * @param request
  *     The request passed to the ioctl(2) system call.
  * @param data
  *     The data passed to the ioctl(2) system call.
  * @returns
  *     a pointer to an iocontrol object, that may be used to describe
  *     the call the ioctl(2) system call.
  */
const explain_iocontrol_t *explain_iocontrol_find_by_number(int fildes,
    int request, const void *data);

/**
  * The explain_iocontrol_request_by_name function may be used to
  * locate an ioctl by name.
  *
  * @param name
  *     The name of the ioctl(2) request.
  * @returns
  *     a pointer to an iocontrol object, that may be used to describe
  *     the call the ioctl(2) system call; or NULL if not found.
  */
const explain_iocontrol_t *explain_iocontrol_request_by_name(const char *name);

/**
  * The explain_iocontrol_print_name function is used to print the
  * name of a request argument passed to an ioctl(2) system call.
  *
  * @param p
  *     Pointer to the explain_iocontrol_t instance,
  *     rather like "this" in C++ code.
  * @param sb
  *     The string buffer to print into.
  * @param errnum
  *     The error number that caused all this, obtained from the
  *     global errno variable, possibly indirectly.
  * @param fildes
  *     The original fildes, exactly as passed to the ioctl(2) system call.
  * @param request
  *     The original request, exactly as passed to the ioctl(2) system call.
  * @param data
  *     The original data, exactly as passed to the ioctl(2) system call.
  */
void explain_iocontrol_print_name(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes,
    int request, const void *data);

/**
  * The explain_iocontrol_print_data function is used to print a
  * representation of the data argument passed to an ioctl(2) system
  * call.
  *
  * @param p
  *     Pointer to the explain_iocontrol_t instance,
  *     rather like "this" in C++ code.
  * @param sb
  *     The string buffer to print into.
  * @param errnum
  *     The error number that caused all this, obtained from the
  *     global errno variable, possibly indirectly.
  * @param fildes
  *     The original fildes, exactly as passed to the ioctl(2) system call.
  * @param request
  *     The original request, exactly as passed to the ioctl(2) system call.
  * @param data
  *     The original data, exactly as passed to the ioctl(2) system call.
  */
void explain_iocontrol_print_data(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes,
    int request, const void *data);

/**
  * The explain_iocontrol_print_explanation function is used to print
  * an explanation for an error reported by an ioctl(2) system call.
  *
  * @param p
  *     Pointer to the explain_iocontrol_t instance,
  *     rather like "this" in C++ code.
  * @param sb
  *     The string buffer to print into.
  * @param errnum
  *     The error number that caused all this, obtained from the
  *     global errno variable, possibly indirectly.
  * @param fildes
  *     The original fildes, exactly as passed to the ioctl(2) system call.
  * @param request
  *     The original request, exactly as passed to the ioctl(2) system call.
  * @param data
  *     The original data, exactly as passed to the ioctl(2) system call.
  */
void explain_iocontrol_print_explanation(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes,
    int request, const void *data);

/**
  * The explain_parse_ioctl_request_or_die function is used to parse
  * a text string to produce an ioctl(2) request value.
  *
  * @param text
  *     The text string to be parsed.
  * @returns
  *     On success, the ioctl(2) request value; on failure, this
  *     function does not return, but instead prints an error message
  *     and terminates via exit(2).
  */
int explain_parse_ioctl_request_or_die(const char *text);

/**
  * The explain_iocontrol_statistics function is used to obtaion
  * statistics about the ioctl commands supported by libexplain.
  *
  * @param total
  *     The total number of ioctls understood by libexplain.
  * @param active
  *     The number of ioctls relevant to this system.
  *     assert(active <= total)
  */
void explain_iocontrol_statistics(int *total, int *active);

/**
  * The explain_iocontrol_check_conflicts function is sued to verify
  * that there are no un-expected ioctl request number conflicts.
  * (Expected conflicts have disambiguate functions defined.)
  */
void explain_iocontrol_check_conflicts(void);

/**
  * For use by individual ioctl handlers, a disambiguation that always
  * reports success (0).  See explain_iocontrol.disambiguate, above.
  */
int explain_iocontrol_disambiguate_true(int fildes, int request,
    const void *data);

/**
  * For use by individual ioctl handlers, a disambiguation that always
  * reports failure (-1).  See explain_iocontrol.disambiguate, above.
  */
int explain_iocontrol_disambiguate_false(int fildes, int request,
    const void *data);

/**
  * For use by individual ioctl handlers, a disambiguation that
  * reports success (0) for sockets, and failure (-1) otherwise.
  */
int explain_iocontrol_disambiguate_is_a_socket(int fildes, int request,
    const void *data);

/**
  * For use by individual ioctl handlers, a disambiguation that
  * reports failure (-1) for sockets, and success (0) otherwise.
  */
int explain_iocontrol_disambiguate_is_not_a_socket(int fildes, int request,
    const void *data);

/**
  * For use by individual ioctl handlers, a disambiguation that reports
  * failure (-1) for almost everything, and success (0) for "eql"
  * network devices.
  */
int explain_iocontrol_disambiguate_is_if_eql(int fildes, int request,
    const void *data);

/**
  * For use by individual ioctl handlers, a disambiguation that reports
  * failure (-1) for almost everything, and success (0) for "eql"
  * network devices.
  */
int explain_iocontrol_disambiguate_is_v4l2(int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_disambiguate_scc function is used to test for
  * a Z8530 SCC device, i.e. "scc" network devices.
  *
  * @param fildes
  *     The file descriptor to test,
  * @param request
  *     probably not relevant
  * @param data
  *     probably not relevant
  * @returns
  *     0 if is a Z8530 device, -1 if anything else.
  */
int explain_iocontrol_disambiguate_scc(int fildes, int request,
    const void *data);

/**
  * The explain_iocontrol_disambiguate_net_dev_name helper function is
  * used to decide whether or not a file descriptor is associated with a
  * network device of the given name.
  *
  * @param fildes
  *     The file descriptor if interest
  * @param name
  *     The name of the network device of interest.  The actual network
  *     device could also have an optional trailing number (e.g.
  *     name="eth" will also match "eth0").
  * @returns
  *     DISAMBIGUATE_USE if the name matches, or
  *     DISAMBIGUATE_DO_NOT_USE if the name does not match.
  */
int explain_iocontrol_disambiguate_net_dev_name(int fildes, const char *name);

/**
  * The explain_iocontrol_check_conflicts function is sued to verify
  * that there are no un-expected ioctl request number conflicts.
  * (Expected conflicts have disambiguate functions defined.)
  */
void explain_iocontrol_check_conflicts(void);

/**
  * The NOT_A_POINTER value is assigned to an iocontrol::data_size
  * member to indicate that a given ioctl does not take a pointer
  * argument.
  */
#define NOT_A_POINTER (unsigned)(-1)

#define VOID_STAR (unsigned)(-2)

/**
  * The IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE flag value indicares the the
  * size in the define from IOC(a,b,c) does not correspond to the actual
  * data type used in the kernel.
  */
#define IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE  0x0001

/**
  * The IOCONTROL_FLAG_NON_META flag value indicates that the request
  * is just a random number, and was not defined using the _IOC(a,b,c)
  * macro, and thus it contains no useful meta-data for checking size
  * and direction.
  */
#define IOCONTROL_FLAG_NON_META             0x0002

/**
  * The IOCONTROL_FLAG_RW flag is used to indicate that _IOR() is
  * actually _IORW() in behaviour.
  */
#define IOCONTROL_FLAG_RW                   0x0004

/**
  * The DISAMBIGUATE_USE symbol is used to indicate that a disambiguate
  * function returns an OK result, the iocontrol entry can be used.
  */
#define DISAMBIGUATE_USE 0

/**
  * The DISAMBIGUATE_DO_NOT_USE symbol is used to indicate that a
  * disambiguate function returns a negative result, the iocontrol entry
  * shall not be used.
  */
#define DISAMBIGUATE_DO_NOT_USE -1

#endif /* LIBEXPLAIN_IOCONTROL_H */
/* vim: set ts=8 sw=4 et : */
