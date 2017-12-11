/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/net/if_ppp.h>

#include <libexplain/iocontrol/disambiguate/if_ppp.h>


int
explain_iocontrol_disambiguate_is_if_ppp(int fildes, int request,
    const void *data)
{
#ifdef SIOCGPPPSTATS
    int             debug;

    (void)data;

    /*
     * Eliminate as much as possible without any system calls at all.
     */
    switch (request)
    {
    case SIOCGPPPCSTATS:
    case SIOCGPPPSTATS:
    case SIOCGPPPVER:
        break;

    default:
        return DISAMBIGUATE_DO_NOT_USE;
    }

    /*
     * We ask the file descriptor for its PPP debug level.
     * If it barfs, this file descriptor isn't suitable.
     */
    debug = 0;
    if (ioctl(fildes, PPPIOCGDEBUG, &debug) < 0)
        return DISAMBIGUATE_DO_NOT_USE;

    /* looks ok */
    return DISAMBIGUATE_USE;
#else
    (void)fildes;
    (void)request;
    (void)data;
    return DISAMBIGUATE_DO_NOT_USE;
#endif
}


/* vim: set ts=8 sw=4 et : */
