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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/scc.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/intptr_t.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/siocsccini.h>

#ifdef HAVE_LINUX_SCC_H


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_intptr_t(sb, (intptr_t)data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EPERM:
        explain_buffer_dac_sys_rawio(sb);
        break;

    case EINVAL:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an
             * EINVAL error reported by the ioctl SIOCSCCINI system
             * call.
             */
            i18n("you must call ioctl SIOCSCCCFG first")
        );
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


const explain_iocontrol_t explain_iocontrol_siocsccini =
{
    "SIOCSCCINI", /* name */
    SIOCSCCINI, /* value */
    explain_iocontrol_disambiguate_scc,
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    NOT_A_POINTER, /* data_size */
    "intptr_t", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef SIOCSCCINI */

const explain_iocontrol_t explain_iocontrol_siocsccini =
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

#endif /* SIOCSCCINI */


/* vim: set ts=8 sw=4 et : */
