/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013, 2014 Peter Miller
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
#include <libexplain/ac/linux/fs.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/ebusy.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/iocontrol/blkbszset.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/string_buffer.h>


#ifdef BLKBSZSET

/*
 * On the subject of data_size, the define is
 *
 *     #define BLKBSZGET _IOR(0x12, 112, size_t)
 *
 * however the kernel actually uses "int" internally.
 * So which is correct?  Guess the kernel is.
 */

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int_star(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EACCES:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This error message is issued when ioctl
             * BLKBSZSET returns an EACCES error.
             */
            i18n("the process does not have permission to change the "
                "logical block size")
        );
        explain_buffer_dac_sys_admin(sb);
        break;

    case EBUSY:
        explain_buffer_ebusy_fildes(sb, fildes, "fildes", "ioctl BLKBSZSET");
        break;

    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }
        /* fall through... */

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


const explain_iocontrol_t explain_iocontrol_blkbszset =
{
    "BLKBSZSET", /* name */
    BLKBSZSET, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(int), /* data_size */
    "int *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef BLKBSZSET */

const explain_iocontrol_t explain_iocontrol_blkbszset =
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

#endif /* BLKBSZSET */


/* vim: set ts=8 sw=4 et : */
