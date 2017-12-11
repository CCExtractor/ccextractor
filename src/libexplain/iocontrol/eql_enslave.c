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
#include <libexplain/buffer/ifreq/slaving_request.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/disambiguate/if_ppp.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/eql_enslave.h>
#include <libexplain/is_efault.h>

#ifdef EQL_ENSLAVE


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ifreq_slaving_request(sb, data, 1);
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

    case EFAULT:
        {
            const struct ifreq *i;
            const struct slaving_request *s;

            i = data;
            if (explain_is_efault_pointer(i, sizeof(*i)))
            {
                explain_buffer_efault(sb, "data");
                break;
            }

            s = (const struct slaving_request *)i->ifr_data;
            if (explain_is_efault_pointer(s, sizeof(*s)))
            {
                explain_buffer_efault(sb, "data->ifr_data");
                break;
            }
        }
        goto generic;

    case ENOMEM:
        goto generic;

    case EINVAL:
        /*
         * FIXME: not suitable: the slave may not exist, it could
         * already be a slave, it could already be a master, the master
         * interface may not be IFF_UP.
         */
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


const explain_iocontrol_t explain_iocontrol_eql_enslave =
{
    "EQL_ENSLAVE", /* name */
    EQL_ENSLAVE, /* value */
    explain_iocontrol_disambiguate_is_if_ppp,
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

#else /* ndef EQL_ENSLAVE */

const explain_iocontrol_t explain_iocontrol_eql_enslave =
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

#endif /* EQL_ENSLAVE */


/* vim: set ts=8 sw=4 et : */
