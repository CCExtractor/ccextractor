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
#include <libexplain/ac/linux/fs.h>

#include <libexplain/iocontrol/fs_ioc32_setversion.h>
#include <libexplain/iocontrol/generic.h>


#ifdef FS_IOC32_SETVERSION

static int
disambiguate(int fildes, int request, const void *data)
{
    (void)fildes;
    (void)request;
    (void)data;
    /* success = 0, failure = -1 */
    return (FS_IOC32_SETVERSION == FS_IOC_SETVERSION ? -1 : 0);
    /* We can't use #if because the definitions use sizeof. */
}


const explain_iocontrol_t explain_iocontrol_fs_ioc32_setversion =
{
    "FS_IOC32_SETVERSION", /* name */
    FS_IOC32_SETVERSION, /* value */
    disambiguate,
    0, /* print_name */
    explain_iocontrol_generic_print_data_int_star, /* print_data */
    0, /* print_explanation */
    0, /* print_data_returned */
    sizeof(int), /* data_size */
    "int *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef FS_IOC32_SETVERSION */

const explain_iocontrol_t explain_iocontrol_fs_ioc32_setversion =
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

#endif /* FS_IOC32_SETVERSION */


/* vim: set ts=8 sw=4 et : */
