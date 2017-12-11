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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_framebuffer.h>
#include <libexplain/buffer/v4l2_pixel_format.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_fbuf.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_FBUF


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_framebuffer(sb, data);
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
            struct v4l2_framebuffer g;

            if (ioctl(fildes, VIDIOC_G_FBUF, &g) < 0 && errno == EPERM)
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "the device does support output frame buffers"
                );
                return;
            }
        }

        {
            const struct v4l2_framebuffer *arg;

            arg = data;
            if (explain_is_efault_pointer(arg, sizeof(*arg)))
                goto generic;
            switch (explain_v4l2_pixel_format_check(arg->fmt.pixelformat))
            {
            case explain_v4l2_pixel_format_check_ok:
                break;

            case explain_v4l2_pixel_format_check_nosuch:
            default:
                explain_buffer_einval_vague(sb, "data->fmt.pixelformat");
                return;
            }
        }

        explain_buffer_einval_vague(sb, "data");
        return;

    case EPERM:
        if (!explain_capability_sys_admin() && !explain_capability_sys_rawio())
        {
            explain_buffer_eperm_vague(sb, "ioctl VIDIOC_S_FBUF");
            explain_buffer_dac_sys_rawio(sb);
            return;
        }

        /* no idea */
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


const explain_iocontrol_t explain_iocontrol_vidioc_s_fbuf =
{
    "VIDIOC_S_FBUF", /* name */
    VIDIOC_S_FBUF, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_framebuffer), /* data_size */
    "struct v4l2_framebuffer *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_FBUF */

const explain_iocontrol_t explain_iocontrol_vidioc_s_fbuf =
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

#endif /* VIDIOC_S_FBUF */

/* vim: set ts=8 sw=4 et : */
