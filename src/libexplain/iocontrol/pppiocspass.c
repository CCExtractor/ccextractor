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
#include <libexplain/ac/linux/filter.h>
#include <libexplain/ac/linux/if_ppp.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/sock_fprog.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/pppiocspass.h>
#include <libexplain/is_efault.h>

#ifdef PPPIOCSPASS


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_sock_fprog(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EFAULT:
        {
            const struct sock_fprog *prog;

            prog = data;
            if (explain_is_efault_pointer(prog, sizeof(*prog)))
                explain_buffer_efault(sb, "data");
            else
                explain_buffer_efault(sb, "data->filter");
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case EINVAL:
        /*
         * from Linux kernel net/core/filter.c
         *
         * "Check the user's filter code. If we let some ugly
         * filter code slip through kaboom! The filter must contain
         * no references or jumps that are out of range, no illegal
         * instructions, and must end with a RET instruction."
         */
        explain_buffer_einval_sock_fprog(sb, data);
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


const explain_iocontrol_t explain_iocontrol_pppiocspass =
{
    "PPPIOCSPASS", /* name */
    PPPIOCSPASS, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct sock_fprog), /* data_size */
    "struct sock_fprog *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef PPPIOCSPASS */

const explain_iocontrol_t explain_iocontrol_pppiocspass =
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

#endif /* PPPIOCSPASS */


/* vim: set ts=8 sw=4 et : */
