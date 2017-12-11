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
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_requestbuffers.h>
#include <libexplain/buffer/v4l2_memory.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_reqbufs.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_REQBUFS


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_requestbuffers(sb, data);
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
            const struct v4l2_requestbuffers *arg;

            arg = data;
            if (explain_is_efault_pointer(arg, sizeof(arg)))
                goto generic;

            /*
             * Check the type (the Linux kernel always does this first).
             */
            switch (explain_v4l2_buf_type_check(fildes, arg->type))
            {
            case explain_v4l2_buf_type_check_no_idea:
            default:
                goto generic;

            case explain_v4l2_buf_type_check_notsup:
                explain_buffer_enotsup_device(sb, "data->type");
                return;

            case explain_v4l2_buf_type_check_nosuch:
                explain_buffer_einval_vague(sb, "data->type");
                return;

            case explain_v4l2_buf_type_check_ok:
                break;
            }

            /*
             * Check the memory mode.
             */
            switch (arg->memory)
            {
            case V4L2_MEMORY_MMAP:
            case V4L2_MEMORY_USERPTR:
            case V4L2_MEMORY_OVERLAY:
                break;

            default:
                explain_buffer_einval_vague(sb, "data->memory");
                return;
            }
        }

        /* a count of 0 is valid */

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


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_requestbuffers(sb, data);
}


const explain_iocontrol_t explain_iocontrol_vidioc_reqbufs =
{
    "VIDIOC_REQBUFS", /* name */
    VIDIOC_REQBUFS, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(struct v4l2_requestbuffers), /* data_size */
    "struct v4l2_requestbuffers *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_REQBUFS */

const explain_iocontrol_t explain_iocontrol_vidioc_reqbufs =
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

#endif /* VIDIOC_REQBUFS */

/* vim: set ts=8 sw=4 et : */
