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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/net/if_ppp.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/pppiocgasyncmap.h>

#ifdef PPPIOCGASYNCMAP


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EPERM:
        explain_buffer_eperm_net_admin(sb, "ioctl PPPIOCGASYNCMAP");
        break;

    default:
        explain_iocontrol_generic_print_explanation
        (
            p,
            sb,
            errnum,
            fildes,
            request,
            data
        );
        break;
    }
}


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int_star(sb, data);
    /* The Linux kernel says "Async map stuff - all dummy to please pppd" */
}


const explain_iocontrol_t explain_iocontrol_pppiocgasyncmap =
{
    "PPPIOCGASYNCMAP", /* name */
    PPPIOCGASYNCMAP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    explain_iocontrol_generic_print_data_pointer, /* print_data */
    print_explanation,
    print_data_returned,
    sizeof(int), /* data_size */
    "int *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef PPPIOCGASYNCMAP */

const explain_iocontrol_t explain_iocontrol_pppiocgasyncmap =
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

#endif /* PPPIOCGASYNCMAP */


/* vim: set ts=8 sw=4 et : */
