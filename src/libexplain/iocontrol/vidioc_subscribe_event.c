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
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_event_subscription.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_subscribe_event.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_SUBSCRIBE_EVENT


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_v4l2_event_subscription(sb, data);
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
            const struct v4l2_event_subscription *arg;

            arg = data;
            if (!explain_is_efault_pointer(arg, sizeof(*arg)))
            {
                switch (arg->type)
                {
                case V4L2_EVENT_ALL:
                case V4L2_EVENT_VSYNC:
                case V4L2_EVENT_EOS:
                    break;

                default:
                    if (arg->type < V4L2_EVENT_PRIVATE_START)
                    {
                        explain_buffer_einval_vague(sb, "data->type");
                        return;
                    }
                    break;
                }
            }
        }

        /*
         * FIXME: which one?
         * the ioctl may not be supported
         * the type may not be supported, per device
         */
        errnum = ENOTTY;
        /* Fall through... */

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


const explain_iocontrol_t explain_iocontrol_vidioc_subscribe_event =
{
    "VIDIOC_SUBSCRIBE_EVENT", /* name */
    VIDIOC_SUBSCRIBE_EVENT, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_event_subscription), /* data_size */
    "struct v4l2_event_subscription *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

const explain_iocontrol_t explain_iocontrol_vidioc_unsubscribe_event =
{
    "VIDIOC_UNSUBSCRIBE_EVENT", /* name */
    VIDIOC_UNSUBSCRIBE_EVENT, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(struct v4l2_event_subscription), /* data_size */
    "struct v4l2_event_subscription *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_SUBSCRIBE_EVENT */

const explain_iocontrol_t explain_iocontrol_vidioc_subscribe_event =
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

const explain_iocontrol_t explain_iocontrol_vidioc_unsubscribe_event =
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

#endif /* VIDIOC_SUBSCRIBE_EVENT */

/* vim: set ts=8 sw=4 et : */
