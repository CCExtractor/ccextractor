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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/scc_kiss_cmd.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/siocsccgkiss.h>
#include <libexplain/is_efault.h>

#ifdef HAVE_LINUX_SCC_H


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_scc_kiss_cmd(sb, data, 0);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EFAULT:
        efault:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if (explain_is_efault_pointer(data, sizeof(struct scc_kiss_cmd)))
            goto efault;
        /* Fall through... */

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
    explain_buffer_scc_kiss_cmd(sb, data, 1);
}


const explain_iocontrol_t explain_iocontrol_siocsccgkiss =
{
    "SIOCSCCGKISS", /* name */
    SIOCSCCGKISS, /* value */
    explain_iocontrol_disambiguate_scc,
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(struct scc_kiss_cmd), /* data_size */
    "struct scc_kiss_cmd *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef SIOCSCCGKISS */

const explain_iocontrol_t explain_iocontrol_siocsccgkiss =
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

#endif /* SIOCSCCGKISS */


/* vim: set ts=8 sw=4 et : */
