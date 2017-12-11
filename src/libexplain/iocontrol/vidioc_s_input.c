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
#include <libexplain/buffer/v4l2_input.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_input.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_INPUT


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)request;
    explain_buffer_v4l2_input_index_ptr(sb, data, fildes);
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
            const int       *arg;
            int             ninputs;

            arg = data;
            if (explain_is_efault_pointer(arg, sizeof(*arg)))
                goto generic;

            ninputs = explain_v4l2_input_get_ninputs(fildes);
            if (ninputs == 0)
            {
                errnum = ENOTTY;
                goto generic;
            }
            if (ninputs > 0)
            {
                if (*arg < 0 || *arg >= ninputs)
                {
                    explain_buffer_einval_out_of_range
                    (
                        sb,
                        "*data",
                        0,
                        ninputs - 1
                    );
                    return;
                }
            }
        }

        generic:
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


const explain_iocontrol_t explain_iocontrol_vidioc_s_input =
{
    "VIDIOC_S_INPUT", /* name */
    VIDIOC_S_INPUT, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data, /* print_data_returned */
    sizeof(int), /* data_size */
    "int *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_INPUT */

const explain_iocontrol_t explain_iocontrol_vidioc_s_input =
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

#endif /* VIDIOC_S_INPUT */

/* vim: set ts=8 sw=4 et : */
