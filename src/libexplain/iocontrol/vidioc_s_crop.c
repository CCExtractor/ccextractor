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
#include <libexplain/buffer/enotsup.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_crop.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_crop.h>

#ifdef VIDIOC_S_CROP


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_crop(sb, data, 1);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    /*
     * Note: Linux:
     * drivers/media/video/sh_vou.c can return EIO when it means EINVAL
     */
    switch (errnum)
    {
    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }

        switch (explain_v4l2_crop_check_type(fildes, data))
        {
        case explain_v4l2_crop_check_type_no_idea:
        default:
            goto generic;

        case explain_v4l2_crop_check_type_notsup:
            explain_buffer_enotsup_device(sb, "data->type");
            break;

        case explain_v4l2_crop_check_type_nosuch:
            explain_buffer_einval_vague(sb, "data->type");
            break;

        case explain_v4l2_crop_check_type_ok:
            /*
             * FIXME: which of left, top, width, height?
             * Sometimes it will be an alignment issue,
             * e.g. RGGB and YYUV devices demand even alignment.
             */
            explain_buffer_einval_vague(sb, "data->c");
            break;
        }
        break;

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


const explain_iocontrol_t explain_iocontrol_vidioc_s_crop =
{
    "VIDIOC_S_CROP", /* name */
    VIDIOC_S_CROP, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_crop), /* data_size */
    "struct v4l2_crop *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_CROP */

const explain_iocontrol_t explain_iocontrol_vidioc_s_crop =
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

#endif /* VIDIOC_S_CROP */

/* vim: set ts=8 sw=4 et : */
