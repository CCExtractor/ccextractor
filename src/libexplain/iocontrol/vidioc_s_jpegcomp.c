/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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
#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_jpegcompression.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_jpegcomp.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_JPEGCOMP


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_jpegcompression(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }

        {
            struct v4l2_jpegcompression qry;

            /*
             * First, check that JPEG compression support is actually present.
             *
             * It is possible that VIDIOC_G_JPEGCOMP is supported but
             * VIDIOC_S_JPEGCOMP is not, but none of the drivers I looked
             * at were asymetric in this way.
             */
            if (ioctl(fildes, VIDIOC_G_JPEGCOMP, &qry) < 0 && errno == EINVAL)
            {
                explain_buffer_enosys_fildes
                (
                    sb,
                    fildes,
                    "fildes",
                    "ioctl VIDIOC_S_JPEGCOMP"
                );
                return;
            }
        }

        {
            const struct v4l2_jpegcompression *arg;

            arg = data;
            if (!explain_is_efault_pointer(arg, sizeof(*arg)))
            {
                if (arg->quality < 0 || arg->quality > 100)
                {
                    /* Some drivers are fussier than this. */
                    explain_buffer_einval_out_of_range
                    (
                        sb,
                        "data->quality",
                        0,
                        100
                    );
                    return;
                }

                if (arg->APPn < 0 || arg->APPn > 15)
                {
                    explain_buffer_einval_out_of_range(sb, "data->APPn", 0, 15);
                    return;
                }

                if
                (
                    arg->APP_len < 0
                ||
                    arg->APP_len > (int)sizeof(arg->APP_data)
                )
                {
                    explain_buffer_einval_out_of_range
                    (
                        sb,
                        "data->APP_len",
                        0,
                        sizeof(arg->APP_data)
                    );
                    return;
                }

                if
                (
                    arg->COM_len < 0
                ||
                    arg->COM_len > (int)sizeof(arg->COM_data)
                )
                {
                    explain_buffer_einval_out_of_range
                    (
                        sb,
                        "data->COM_len",
                        0,
                        sizeof(arg->COM_data)
                    );
                    return;
                }
            }
        }

        /* Nothing obvious */
        explain_buffer_einval_vague(sb, "data");
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


const explain_iocontrol_t explain_iocontrol_vidioc_s_jpegcomp =
{
    "VIDIOC_S_JPEGCOMP", /* name */
    VIDIOC_S_JPEGCOMP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_jpegcompression), /* data_size */
    "struct v4l2_jpegcompression *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_JPEGCOMP */

const explain_iocontrol_t explain_iocontrol_vidioc_s_jpegcomp =
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

#endif /* VIDIOC_S_JPEGCOMP */

/* vim: set ts=8 sw=4 et : */
