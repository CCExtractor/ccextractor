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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/cdrom.h>
#include <libexplain/ac/stdint.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/long.h>
#include <libexplain/iocontrol/cdrom_debug.h>
#include <libexplain/iocontrol/generic.h>


#ifdef CDROM_DEBUG

static void
print_explanation(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    switch (errnum)
    {
    case EACCES:
        /* FIXME: it could be a DVD */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain and
             * EACCES error reported by a CDROM_DEBUG ioctl.
             */
            i18n("the process does not have permission to change the "
                "CD-ROM debugging flag")
        );
        explain_buffer_dac_sys_admin(sb);
        break;

    default:
        explain_iocontrol_generic.print_explanation
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


const explain_iocontrol_t explain_iocontrol_cdrom_debug =
{
    "CDROM_DEBUG", /* name */
    CDROM_DEBUG, /* value */
    0, /* disambiguate */
    0, /* print_name */
    explain_iocontrol_generic_print_data_long,
    print_explanation,
    0, /* print_data_returned */
    NOT_A_POINTER, /* data_size */
    "intptr_t", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef CDROM_DEBUG */

const explain_iocontrol_t explain_iocontrol_cdrom_debug =
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

#endif /* CDROM_DEBUG */


/* vim: set ts=8 sw=4 et : */
