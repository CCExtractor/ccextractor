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
#include <libexplain/ac/linux/net_tstamp.h>
#include <libexplain/ac/linux/sockios.h>
#include <libexplain/ac/net/if.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/linux/net_tstamp.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/ifreq_data/hwtstamp_config.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/siocshwtstamp.h>
#include <libexplain/is_efault.h>

#ifdef SIOCSHWTSTAMP


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ifreq_data_hwtstamp_config(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EPERM:
        if (!explain_capability_net_admin())
        {
            explain_buffer_eperm_net_admin(sb, "ioctl SIOCSHWTSTAMP");
            explain_buffer_dac_net_admin(sb);
            break;
        }
        goto generic;

#ifdef HAVE_LINUX_NET_TSTAMP_H
    case EINVAL:
        {
            const struct ifreq *rq;

            rq = data;
            if (!explain_is_efault_pointer(rq, sizeof(*rq)))
            {
                const struct hwtstamp_config *cfg;

                cfg = (void *)rq->ifr_data;
                if
                (
                    !explain_is_efault_pointer(cfg, sizeof(*cfg))
                &&
                    cfg->flags != 0
                )
                {
                    explain_buffer_einval_vague(sb, "data->ifr_data->flags");
                    break;
                }
            }
         }
         goto generic;

    case ERANGE:
         {
            const struct ifreq *rq;

            rq = data;
            if (!explain_is_efault_pointer(rq, sizeof(*rq)))
            {
                const struct hwtstamp_config *cfg;

                cfg = (void *)rq->ifr_data;
                if (!explain_is_efault_pointer(cfg, sizeof(*cfg)))
                {
                    int             ok;

                    switch (cfg->tx_type)
                    {
                    case HWTSTAMP_TX_OFF:
                    case HWTSTAMP_TX_ON:
                        ok = 1;
                        break;

                    default:
                        ok = 0;
                        break;
                    }
                    if (!ok)
                    {
                        explain_buffer_einval_vague
                        (
                            sb,
                            "data->ifr_data->tx_type"
                        );
                        return;
                    }

                    switch (cfg->rx_filter)
                    {
                    case HWTSTAMP_FILTER_NONE:
                    case HWTSTAMP_FILTER_ALL:
                    case HWTSTAMP_FILTER_SOME:
                    case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
                    case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
                    case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
                    case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
                    case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
                    case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
                    case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
                    case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
                    case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
                    case HWTSTAMP_FILTER_PTP_V2_EVENT:
                    case HWTSTAMP_FILTER_PTP_V2_SYNC:
                    case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
                        ok = 1;
                        break;

                    default:
                        ok = 0;
                    }
                    if (!ok)
                    {
                        explain_buffer_einval_vague
                        (
                            sb,
                            "data->ifr_data->rx_filter"
                        );
                        return;
                    }
                }
            }
        }
        goto generic;

#endif

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


const explain_iocontrol_t explain_iocontrol_siocshwtstamp =
{
    "SIOCSHWTSTAMP", /* name */
    SIOCSHWTSTAMP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data, /* print_data_returned */
    sizeof(struct ifreq), /* data_size */
    "struct ifreq *", /* data_type */
    IOCONTROL_FLAG_NON_META, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef SIOCSHWTSTAMP */

const explain_iocontrol_t explain_iocontrol_siocshwtstamp =
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

#endif /* SIOCSHWTSTAMP */


/* vim: set ts=8 sw=4 et : */
