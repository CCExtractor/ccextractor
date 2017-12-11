/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/vt.h>
#include <libexplain/ac/stdint.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/vt_mode.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vt_reldisp.h>

#ifdef VT_RELDISP


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_vt_mode_mode(sb, (intptr_t)data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EPERM:
        if (!explain_capability_sys_tty_config())
        {
            explain_buffer_eperm_sys_tty_config(sb, "ioctl VT_RELDISP");
            break;
        }
        goto generic;

    case EINVAL:
        explain_buffer_einval_vague(sb, "data");
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


const explain_iocontrol_t explain_iocontrol_vt_reldisp =
{
    "VT_RELDISP", /* name */
    VT_RELDISP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    NOT_A_POINTER, /* data_size */
    "intptr_t", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VT_RELDISP */

const explain_iocontrol_t explain_iocontrol_vt_reldisp =
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

#endif /* VT_RELDISP */


/* vim: set ts=8 sw=4 et : */
