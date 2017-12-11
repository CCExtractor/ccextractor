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
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/loff_t.h>
#include <libexplain/buffer/wrong_file_type.h>
#include <libexplain/iocontrol/fioqsize.h>
#include <libexplain/iocontrol/generic.h>


#ifdef FIOQSIZE

static void
print_data_returned(const explain_iocontrol_t *p,
    struct explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_loff_t_star(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    switch (errnum)
    {
    case ENOTTY:
        {
            struct stat st;
            if
            (
                fstat(fildes, &st) >= 0
            &&
                !S_ISDIR(st.st_mode)
            &&
                !S_ISREG(st.st_mode)
            &&
                !S_ISLNK(st.st_mode)
            )
            {
                explain_buffer_wrong_file_type_st
                (
                    sb,
                    &st,
                    "fildes",
                    S_IFREG
                );
                break;
            }
        }
        /* Fall through... */

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


const explain_iocontrol_t explain_iocontrol_fioqsize =
{
    "FIOQSIZE", /* name */
    FIOQSIZE, /* value */
    0, /* disambiguate */
    0, /* print_name */
    explain_iocontrol_generic_print_data_pointer, /* print_data */
    print_explanation,
    print_data_returned,
    sizeof(loff_t), /* data_size */
    "loff_t *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__
};

#else

const explain_iocontrol_t explain_iocontrol_fioqsize =
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
    __LINE__
};

#endif


/* vim: set ts=8 sw=4 et : */
