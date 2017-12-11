/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_GCC_ATTRIBUTES_H
#define LIBEXPLAIN_GCC_ATTRIBUTES_H

#ifdef __GNUC__
#define LIBEXPLAIN_NORETURN __attribute__((noreturn))
#define LIBEXPLAIN_LINKAGE_HIDDEN __attribute__((visibility("hidden")))
#define LIBEXPLAIN_FORMAT_PRINTF(x, y) __attribute__((format(printf, x, y)))
#define LIBEXPLAIN_FORMAT_VPRINTF(x) __attribute__((format(printf, x, 0)))
#else
#define LIBEXPLAIN_NORETURN
#define LIBEXPLAIN_LINKAGE_HIDDEN
#define LIBEXPLAIN_FORMAT_PRINTF(x, y)
#define LIBEXPLAIN_FORMAT_VPRINTF(x)
#endif

/*
 * Convenience macros to test the versions of glibc and gcc.
 * Use them like this:
 *    #if LIBEXPLAIB_GNUC_PREREQ (2,8)
 *    ... code requiring gcc 2.8 or later ...
 *    #endif
 * Note - they won't work for gcc1 or glibc1, since the _MINOR macros
 * were not defined way back then.
 */
#if defined __GNUC__ && defined __GNUC_MINOR__
# define LIBEXPLAIN_GNUC_MKREV(maj, min)                              \
    (((maj) << 16) | (min))
# define LIBEXPLAIN_GNUC_PREREQ(pmaj, pmin)                           \
    (                                                                 \
        LIBEXPLAIN_GNUC_MKREV(__GNUC__, __GNUC_MINOR__)               \
    >=                                                                \
        LIBEXPLAIN_GNUC_MKREV(pmaj, pmin)                             \
    )
#else
# define LIBEXPLAIN_GNUC_PREREQ(maj, min) 0
#endif

#if LIBEXPLAIN_GNUC_PREREQ(3, 0)
# define LIBEXPLAIN_ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
#else
# define LIBEXPLAIN_ATTRIBUTE_MALLOC
#endif

#if LIBEXPLAIN_GNUC_PREREQ(4, 3)
# define LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(a1) \
    __attribute__ ((__alloc_size__(a1)))
# define LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE2(a1, a2) \
    __attribute__ ((__alloc_size__(a1, a2)))
#else
# define LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE(a1)
# define LIBEXPLAIN_ATTRIBUTE_ALLOC_SIZE2(a1, a2)
#endif


/*
 * We attach this attribute to functions that should never have their
 * return value ignored.  Amongst other things, this can detect the case
 * where the client has called explain_fubar when they meant to call
 * explain_fubar_or_die instead.
 */
#if LIBEXPLAIN_GNUC_PREREQ(3, 4)
#define LIBEXPLAIN_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
#else
#define LIBEXPLAIN_WARN_UNUSED_RESULT
#endif

#endif /* LIBEXPLAIN_GCC_ATTRIBUTES_H */
/* vim: set ts=8 sw=4 et : */
