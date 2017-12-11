/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/types.h> /* old Linux revs need this */
#include <libexplain/ac/sys/types.h>
#include <libexplain/ac/ustat.h>

#include <libexplain/ustat.h>


const char *
explain_ustat(dev_t dev, struct ustat *ubuf)
{
    return explain_errno_ustat(errno, dev, ubuf);
}


/* vim: set ts=8 sw=4 et : */
