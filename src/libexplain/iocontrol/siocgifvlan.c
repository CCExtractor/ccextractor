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

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/linux/if_vlan.h>
#include <libexplain/ac/linux/sockios.h>
#include <libexplain/ac/sys/sockio.h>

#include <libexplain/buffer/vlan_ioctl_args.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/siocgifvlan.h>


#ifdef SIOCGIFVLAN

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_vlan_ioctl_args(sb, data, 0);
}


static void
print_data_returned(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_vlan_ioctl_args(sb, data, 1);
}


const explain_iocontrol_t explain_iocontrol_siocgifvlan =
{
    "SIOCGIFVLAN", /* name */
    SIOCGIFVLAN, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data, /* print_data */
    0, /* print_explanation */
    print_data_returned,
    sizeof(struct vlan_ioctl_args), /* data_size */
    "struct vlan_ioctl_args *", /* data_type */
    IOCONTROL_FLAG_NON_META, /* flags */
    __FILE__,
    __LINE__,
};

#else

const explain_iocontrol_t explain_iocontrol_siocgifvlan =
{
    0, /* name */
    0, /* value */
    0, /* disambiguate */
    0, /* print_name */
    0, /* print_data */
    0, /* print_explanation */
    0, /* print_data_returned */
    0, /* data_size */
    0, /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#endif


/* vim: set ts=8 sw=4 et : */
