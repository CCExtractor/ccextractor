/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/net/if.h> /* not Solaris */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/sockio.h> /* Solaris */

#include <libexplain/iocontrol.h>


int
explain_iocontrol_disambiguate_net_dev_name(int fildes, const char *name)
{
#if defined(SIOCGIFINDEX) && defined(SIOCGIFNAME)
    struct ifreq    ifr;
    int             interface_index;
    size_t          name_len;
    const char      *cp;

    /*
     * We ask the file descriptor for its network interface index; if it says
     * no, this file descriptor isn't suitable.
     */
    memset(&ifr, 0, sizeof(ifr));
    if (ioctl(fildes, SIOCGIFINDEX, &ifr) < 0)
        return DISAMBIGUATE_DO_NOT_USE;
    interface_index = ifr.ifr_ifindex ;

    /*
     * We ask the file descriptor for its network interface name (based on the
     * interface index); if it says no, this file descriptor isn't suitable.
     */
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_ifindex = interface_index;
    if (ioctl(fildes, SIOCGIFNAME, &ifr) < 0)
        return DISAMBIGUATE_DO_NOT_USE;

    /*
     * Finally, we test the interface name.
     * It could have an optional trailing number.
     */
    name_len = strlen(name);
    if (0 != memcmp(ifr.ifr_name, name, name_len))
        return DISAMBIGUATE_DO_NOT_USE;
    cp = ifr.ifr_name + name_len;
    while (isdigit(*cp))
        ++cp;
    if (*cp)
        return DISAMBIGUATE_DO_NOT_USE;

    /*
     * Looks good.
     */
    return DISAMBIGUATE_USE;
#else
    return DISAMBIGUATE_DO_NOT_USE;
#endif
}


/* vim: set ts=8 sw=4 et : */
