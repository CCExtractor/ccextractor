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

#include <libexplain/ac/linux/fs.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/blkpg_ioctl_arg.h>
#include <libexplain/iocontrol/blkpg.h>


#ifdef BLKPG

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_blkpg_ioctl_arg(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EACCES:
        goto generic;

    case EFAULT:
        {
            const struct blkpg_ioctl_arg *ap;
            const struct blkpg_partition *pp;

            ap = data;
            if (explain_is_efault_pointer(ap, sizeof(*ap)))
            {
                explain_buffer_efault(sb, "data");
                break;
            }
            pp = ap->data;
            if (explain_is_efault_pointer(pp, sizeof(*pp)))
            {
                explain_buffer_efault(sb, "data->data");
                break;
            }
        }
        goto generic;

    case EINVAL:
        {
            const struct blkpg_ioctl_arg *ap;
            const struct blkpg_partition *pp;
            long            n;

            if (!disk_partitionable(fildes))
            if (disk_max_parts(disk) > 1)
            {
                explain_buffer_enosys_fildes(sb, fildes);
                return;
            }

            ap = data;
            switch (ap->op)
            {
            case BLKPG_ADD_PARTITION:
            case BLKPG_DEL_PARTITION:
                pp = ap->data;
                if (pp->pno <= 0)
                {
                    explain_buffer_einval_too_small(sb, "data->pno", pp->pno);
                    return;
                }
                if (pp->start & 511)
                {
                    explain_buffer_einval_multiple(sb, "data->start", 512);
                    return;
                }
                n = pp->start >> 9;
                if (n < 0)
                {
                    explain_buffer_einval_vague(sb, "data->start");
                    return;
                }
                if (pp->length & 511)
                {
                    explain_buffer_einval_multiple(sb, "data->length", 512);
                    return;
                }
                n = pp->length >> 9;
                if (n < 0)
                {
                    explain_buffer_einval_vague(sb, "data->length");
                    return;
                }
                break;

            default:
                explain_buffer_einval_vague(sb, "data->op");
                return;
            }
        }
        goto generic;

    case ENXIO:
        explain_buffer_einval_vague(sb, "data->data->pno");
        break;

    case EBUSY:
        goto generic;

    default:
        generic:
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


const explain_iocontrol_t explain_iocontrol_blkpg =
{
    "BLKPG", /* name */
    BLKPG, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct blkpg_ioctl_arg), /* data_size */
    "struct blkpg_ioctl_arg *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef BLKPG */

const explain_iocontrol_t explain_iocontrol_blkpg =
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

#endif /* BLKPG */


/* vim: set ts=8 sw=4 et : */
