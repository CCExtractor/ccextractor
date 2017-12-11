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
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_hw_freq_seek.h>
#include <libexplain/buffer/v4l2_tuner.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_hw_freq_seek.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_HW_FREQ_SEEK


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)request;
    explain_buffer_v4l2_hw_freq_seek(sb, data, fildes);
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
            const struct v4l2_hw_freq_seek *arg;

            arg = data;
            if (!explain_is_efault_pointer(arg, sizeof(*arg)))
            {
                struct v4l2_tuner qry;
                int             ntuners;

                /*
                 * The tuner may be wrong.
                 */
                ntuners = explain_v4l2_tuner_get_n(fildes);
                if (ntuners == 0)
                {
                    explain_iocontrol_generic_print_explanation
                    (
                        p,
                        sb,
                        ENOTTY,
                        fildes,
                        request,
                        data
                    );
                    return;
                }
                if (ntuners > 0 && arg->tuner >= (unsigned)ntuners)
                {
                    explain_buffer_einval_out_of_range
                    (
                        sb,
                        "data->tuner",
                        0,
                        ntuners - 1
                    );
                    return;
                }

                /*
                 * The type may be valid, but not supported by device.
                 * Specifically, it may not be the corerct type for this tuner.
                 */
                memset(&qry, 0, sizeof(qry));
                qry.index = arg->tuner;
                if
                (
                    ioctl(fildes, VIDIOC_G_TUNER, &qry) >= 0
                &&
                    qry.type != arg->type
                )
                {
                    explain_buffer_einval_vague(sb, "data->type");

                    explain_string_buffer_puts(sb->footnotes, "; ");
                    explain_string_buffer_puts
                    (
                        sb->footnotes,
                        /* FIXME: i18n */
                        "the tuner type is "
                    );
                    explain_buffer_v4l2_buf_type(sb, qry.type);
                    return;
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


const explain_iocontrol_t explain_iocontrol_vidioc_s_hw_freq_seek =
{
    "VIDIOC_S_HW_FREQ_SEEK", /* name */
    VIDIOC_S_HW_FREQ_SEEK, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_hw_freq_seek), /* data_size */
    "struct v4l2_hw_freq_seek *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_HW_FREQ_SEEK */

const explain_iocontrol_t explain_iocontrol_vidioc_s_hw_freq_seek =
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

#endif /* VIDIOC_S_HW_FREQ_SEEK */

/* vim: set ts=8 sw=4 et : */
