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

#include <libexplain/ac/linux/if_eql.h>

#include <libexplain/iocontrol.h>


int
explain_iocontrol_disambiguate_is_if_eql(int fildes, int request,
    const void *data)
{
#ifdef HAVE_LINUX_IF_EQL_H
    /*
     * Eliminate as much as possible without any system calls at all.
     */
    switch (request)
    {
    case EQL_EMANCIPATE:
    case EQL_ENSLAVE:
    case EQL_GETMASTRCFG:
    case EQL_GETSLAVECFG:
    case EQL_SETMASTRCFG:
    case EQL_SETSLAVECFG:
        return explain_iocontrol_disambiguate_net_dev_name(fildes, "eql");

    default:
        return DISAMBIGUATE_DO_NOT_USE;
    }
#else
    (void)fildes;
    (void)request;
#endif
    (void)data;
    return DISAMBIGUATE_DO_NOT_USE;
}


/* vim: set ts=8 sw=4 et : */
