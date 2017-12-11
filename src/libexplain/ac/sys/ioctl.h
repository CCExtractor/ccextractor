/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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

#ifndef LIBEXPLAIN_AC_SYS_IOCTL_H
#define LIBEXPLAIN_AC_SYS_IOCTL_H

/**
  * @file
  * @brief Insulate <sys/ioctl.h> differences
  */

#include <libexplain/config.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <libexplain/ac/sys/ioccom.h>

/*
 * In the _IO _IOW _IOWR _IOR world
 * some have numeric equivalents
 */
#if defined(_IO) && defined(IOC_OUT) && defined(IOC_IN)
#ifndef _IOWN
#define _IOWN(x, y, t) (IOC_IN | ((t) << 16) | ((x) << 8) | y)
#endif
#ifndef _IOWRN
#define _IOWRN(x, y, t) (IOC_INOUT | ((t) << 16) | ((x) << 8) | y)
#endif
#ifndef _IORN
#define _IORN(x, y, t) (IOC_OUT | ((t) << 16) | ((x) << 8) | y)
#endif
#endif

#endif /* LIBEXPLAIN_AC_SYS_IOCTL_H */
/* vim: set ts=8 sw=4 et : */
