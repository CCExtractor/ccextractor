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
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/linux/hayesesp.h>

#include <libexplain/buffer/ebusy.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/hayes_esp_config.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/tiocshayesesp.h>
#include <libexplain/is_efault.h>


#if defined(TIOCSHAYESESP) && defined(HAVE_LINUX_HAYESESP_H)

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_hayes_esp_config(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    switch (errnum)
    {
    case EBUSY:
        explain_buffer_ebusy_fildes
        (
            sb,
            fildes,
            "fildes",
            "ioctl TIOCSHAYESESP"
        );
        break;

#ifdef HAVE_LINUX_HAYESESP_H
    case EINVAL:
        {
            const struct hayes_esp_config *q;

            q = data;
            if (q->flow_on >= q->flow_off)
            {
                explain_buffer_einval_value(sb, "data->flow_on", q->flow_on);
                break;
            }
            if (q->rx_trigger < 1 || q->rx_trigger > 1023)
            {
                explain_buffer_einval_value
                (
                    sb,
                    "data->rx_trigger",
                    q->rx_trigger
                );
                break;
            }
            if (q->tx_trigger > 1023)
            {
                explain_buffer_einval_value
                (
                    sb,
                    "data->tx_trigger",
                    q->tx_trigger
                );
                break;
            }
            if (q->flow_off < 1 || q->flow_off > 1023)
            {
                explain_buffer_einval_value
                (
                    sb,
                    "data->flow_off",
                    q->flow_off
                );
                break;
            }
            if (q->flow_on < 1 || q->flow_on > 1023)
            {
                explain_buffer_einval_value
                (
                    sb,
                    "data->flow_on",
                    q->flow_on
                );
                break;
            }
            if (q->pio_threshold < 1 || q->pio_threshold > 1024)
            {
                explain_buffer_einval_value
                (
                    sb,
                    "data->pio_threshold",
                    q->pio_threshold
                );
                break;
            }
        }
        break;
#endif

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


const explain_iocontrol_t explain_iocontrol_tiocshayesesp =
{
    "TIOCSHAYESESP", /* name */
    TIOCSHAYESESP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct hayes_esp_config), /* data_size */
    "struct hayes_esp_config *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else

const explain_iocontrol_t explain_iocontrol_tiocshayesesp =
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

#endif


/* vim: set ts=8 sw=4 et : */
