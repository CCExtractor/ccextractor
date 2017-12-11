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
#include <libexplain/ac/linux/if_eql.h>
#include <libexplain/ac/net/if.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/ifreq/slave_config.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/eql_setslavecfg.h>
#include <libexplain/is_efault.h>

#ifdef EQL_SETSLAVECFG


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ifreq_slave_config(sb, data, 1);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    /* see Linux kernel sources, drivers/net/eql.c */
    switch (errnum)
    {
    case EPERM:
        if (!explain_capability_net_admin())
        {
            explain_buffer_eperm_net_admin(sb, "ioctl EQL_EMANCIPATE");
            break;
        }
        goto generic;

    case EINVAL:
        /*
         * FIXME: the named slave device does not exist
         */
        goto generic;

    case ENODEV:
        /*
         * FIXME: the name slave device isn't a slave,
         */
        goto generic;

    case EFAULT:
        {
            const struct ifreq *q;
            const struct slaving_request *srqp;

            q = data;
            if (explain_is_efault_pointer(q, sizeof(*q)))
            {
                explain_buffer_efault(sb, "data");
                break;
            }
            srqp = (const struct slaving_request *)q->ifr_data;
            if (explain_is_efault_pointer(srqp, sizeof(*srqp)))
            {
                explain_buffer_efault(sb, "data->ifr_data");
                break;
            }
        }
        goto generic;

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


const explain_iocontrol_t explain_iocontrol_eql_setslavecfg =
{
    "EQL_SETSLAVECFG", /* name */
    EQL_SETSLAVECFG, /* value */
    explain_iocontrol_disambiguate_is_if_eql,
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct ifreq), /* data_size */
    "struct ifreq *", /* data_type */
    IOCONTROL_FLAG_NON_META, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef EQL_SETSLAVECFG */

const explain_iocontrol_t explain_iocontrol_eql_setslavecfg =
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

#endif /* EQL_SETSLAVECFG */


/* vim: set ts=8 sw=4 et : */
