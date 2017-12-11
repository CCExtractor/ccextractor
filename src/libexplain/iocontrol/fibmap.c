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
#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/fs.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/iocontrol/fibmap.h>
#include <libexplain/iocontrol/generic.h>


#ifdef FIBMAP

static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    char            name[40];

    switch (errnum)
    {
    case EINVAL:
        errnum = ENOTTY;
        goto generic;

    case EPERM:
        explain_iocontrol_fake_syscall_name(name, sizeof(name), p, request);
        explain_buffer_eacces_syscall(sb, name);
        explain_buffer_dac_sys_rawio(sb);
        break;

    default:
        generic:
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


const explain_iocontrol_t explain_iocontrol_fibmap =
{
    "FIBMAP", /* name */
    FIBMAP, /* value */
#if defined(BMAP_IOCTL)
    explain_iocontrol_disambiguate_true, /* preferred */
#else
    0, /* disambiguate */
#endif
    0, /* print_name */
    explain_iocontrol_generic_print_data_int_star, /* print_data */
    print_explanation,
    explain_iocontrol_generic_print_data_int_star, /* print_data_returned */
    sizeof(int), /* data_size */
    "int *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef FIBMAP */

const explain_iocontrol_t explain_iocontrol_fibmap =
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

#endif /* FIBMAP */


/* vim: set ts=8 sw=4 et : */
