/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_AC_LINUX_TYPES_H
#define LIBEXPLAIN_AC_LINUX_TYPES_H

#include <libexplain/config.h>

#ifdef HAVE_LINUX_TYPES_H

#   ifdef CAN_INLUDE_BOTH_sys_types_h_AND_linux_types_h
#      include <libexplain/ac/sys/types.h>
#   else
#      define LIBEXPLAIN_AC_SYS_TYPES_H
#   endif

#include <linux/types.h>
#endif

/*
 * This is in the kernel's <linux/types.h> but inside #ifdef __KERNEL__
 * bracketing, which is stripped out to make the user-space <linux/types.h>,
 * but some ding-bat used the type in a user-space struct.
 */
#ifndef aligned_u64
#define aligned_u64 __u64 __attribute__((aligned(8)))
#endif

#endif /* LIBEXPLAIN_AC_LINUX_TYPES_H */
/* vim: set ts=8 sw=4 et : */
