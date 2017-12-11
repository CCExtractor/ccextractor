/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2013, 2014 Peter Miller
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

#ifndef LIBEXPLAIN_CONFIG_MESSY_H
#define LIBEXPLAIN_CONFIG_MESSY_H

/*
 * clang tries to impersonate gcc, but does a poor job of it.
 */
#ifdef __clang__
#undef __GNUC__
#undef __GNU_MINOR__
#endif

/*
 * Make sure Solaris includes POSIX extensions.
 */
#if (defined(__sun) || defined(__sun__) || defined(sun)) && \
        (defined(__svr4__) || defined(svr4))

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#ifndef __EXTENSIONS__
#define __EXTENSIONS__ 1
#endif

/* Make dure FreeBSDbes us everything z*/
#ifndef __USE_GNU
#ifdef _GNU_SOURCE
#define __USE_GNU 1
#endif
#endif

/*
 * fix a glitch in Solaris's <sys/time.h>
 * which only show's up when you turn __EXTENSIONS__ on
 */
#define _timespec timespec      /* fix 2.4 */
#define _tv_sec tv_sec          /* fix 2.5.1 */

/*
 * The Solaris implementation of wait4() is broken.
 * Don't use it.
 */
#undef HAVE_WAIT4

#endif

#endif /* LIBEXPLAIN_CONFIG_MESSY_H */
/* vim: set ts=8 sw=4 et : */
