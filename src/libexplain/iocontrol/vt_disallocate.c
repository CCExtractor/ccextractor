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
#include <libexplain/buffer/gettext.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vt_disallocate.h>

#ifdef VT_DISALLOCATE


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case ENXIO:
        if ((intptr_t)data < 0 || (intptr_t)data > MAX_NR_CONSOLES)
        {
            explain_buffer_einval_out_of_range(sb, "data", 0, MAX_NR_CONSOLES);
            break;
        }
        goto generic;

    case EBUSY:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is used to explain an EBUSY error
             * returned by an ioctl VT_DISALLOCATE system call.
             */
            i18n("the virtual console is still in use")
        );
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


const explain_iocontrol_t explain_iocontrol_vt_disallocate =
{
    "VT_DISALLOCATE", /* name */
    VT_DISALLOCATE, /* value */
    0, /* disambiguate */
    0, /* print_name */
    explain_iocontrol_generic_print_data_long, /* print_data */
    print_explanation,
    0, /* print_data_returned */
    NOT_A_POINTER, /* data_size */
    "intptr_t", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VT_DISALLOCATE */

const explain_iocontrol_t explain_iocontrol_vt_disallocate =
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

#endif /* VT_DISALLOCATE */


/* vim: set ts=8 sw=4 et : */
