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
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotsup.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_dv_preset.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_dv_preset.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_DV_PRESET


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_dv_preset(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EBUSY:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the device is currently streaming data"
        );
        break;

    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }

        {
            struct v4l2_dv_preset qry;

            memset(&qry, 0, sizeof(qry));
            if
            (
                ioctl(fildes, VIDIOC_G_DV_PRESET, &qry) < 0
            &&
                errno == EINVAL
            )
            {
                explain_buffer_enosys_fildes
                (
                    sb,
                    fildes,
                    "fildes",
                    "ioctl VIDIOC_S_DV_PRESET"
                );
                return;
            }
        }

        {
            const struct v4l2_dv_preset *arg;

            arg = data;
            if (!explain_is_efault_pointer(arg, sizeof(*arg)))
            {
                unsigned        idx;

                for (idx = 0; ; ++idx)
                {
                    struct v4l2_dv_enum_preset qry;

                    memset(&qry, 0, sizeof(qry));
                    qry.index = idx;
                    if (ioctl(fildes, VIDIOC_ENUM_DV_PRESETS, &qry) < 0)
                    {
                        if (errno == EINVAL)
                        {
                            explain_buffer_enotsup_device(sb, "data->preset");
                            return;
                        }
                        break;
                    }
                    if (qry.preset == arg->preset)
                    {
                        /*
                         * The value is supported by the device,
                         * there must be some other reson it failed.
                         */
                        break;
                    }
                }
            }
        }

        /* No idea */
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


const explain_iocontrol_t explain_iocontrol_vidioc_s_dv_preset =
{
    "VIDIOC_S_DV_PRESET", /* name */
    VIDIOC_S_DV_PRESET, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data, /* print_data_returned */
    sizeof(struct v4l2_dv_preset), /* data_size */
    "struct v4l2_dv_preset *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_DV_PRESET */

const explain_iocontrol_t explain_iocontrol_vidioc_s_dv_preset =
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

#endif /* VIDIOC_S_DV_PRESET */

/* vim: set ts=8 sw=4 et : */
