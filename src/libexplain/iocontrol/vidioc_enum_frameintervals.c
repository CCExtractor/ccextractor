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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_frmivalenum.h>
#include <libexplain/buffer/v4l2_pixel_format.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_enum_frameintervals.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_ENUM_FRAMEINTERVALS


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_frmivalenum(sb, data, 0);
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
            const struct v4l2_frmivalenum *arg;
            int             nsizes;
            struct v4l2_format qf;

            arg = data;
            if (explain_is_efault_pointer(arg, sizeof(*arg)))
                goto generic;

            /*
             * the index could be out of range
             */
            nsizes = explain_v4l2_frmivalenum_get_n(fildes, arg->pixel_format);
            if (nsizes > 0 && arg->index >= (unsigned)nsizes)
            {
                explain_buffer_einval_out_of_range
                (
                    sb,
                    "data->index",
                    0,
                    nsizes - 1
                );
                return;
            }

            /*
             * the pixel format could be wrong
             */
            memset(&qf, 0, sizeof(qf));
            if
            (
                ioctl(fildes, VIDIOC_G_FMT, &qf) >= 0
            &&
                qf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE
            &&
                arg->pixel_format != qf.fmt.pix.pixelformat
            )
            {
                explain_buffer_einval_vague(sb, "data->pixel_format");

                explain_string_buffer_puts(sb->footnotes, "; ");
                /* FIXME: i18n */
                explain_string_buffer_puts
                (
                    sb->footnotes,
                    "the current pixel format is "
                );
                explain_buffer_v4l2_pixel_format(sb, qf.fmt.pix.pixelformat);
                return;
            }
        }

        /* No idea */
        explain_buffer_einval_vague(sb, "data");
        return;

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


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_frmivalenum(sb, data, 1);
}


const explain_iocontrol_t explain_iocontrol_vidioc_enum_frameintervals =
{
    "VIDIOC_ENUM_FRAMEINTERVALS", /* name */
    VIDIOC_ENUM_FRAMEINTERVALS, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(struct v4l2_frmivalenum), /* data_size */
    "struct v4l2_frmivalenum *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_ENUM_FRAMEINTERVALS */

const explain_iocontrol_t explain_iocontrol_vidioc_enum_frameintervals =
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

#endif /* VIDIOC_ENUM_FRAMEINTERVALS */

/* vim: set ts=8 sw=4 et : */
