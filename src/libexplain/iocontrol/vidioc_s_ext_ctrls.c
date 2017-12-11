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
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_ext_controls.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_s_ext_ctrls.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOC_S_EXT_CTRLS


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)request;
    explain_buffer_v4l2_ext_controls(sb, data, 1, fildes);
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
            const struct v4l2_ext_controls *arg;
            struct v4l2_ext_controls ctrls;
            unsigned        j;

            /*
             * Note: fuzz testing could lead to improbably large item
             * counts, and this next test should catch most of them.
             */
            arg = data;
            if (explain_is_efault_pointer(arg, sizeof(*arg)))
                goto generic;
            if (arg->count)
            {
                size_t          nbytes;

                nbytes = arg->count * sizeof(arg->controls[0]);
                if(explain_is_efault_pointer(arg->controls, nbytes))
                    goto generic;
            }

            /*
             * The control class may be the problem.
             */
            if (arg->count == 0)
                goto bad_ctrl_class;
            memset(&ctrls, 0, sizeof(ctrls));
            ctrls.ctrl_class = arg->ctrl_class;
            if
            (
                ioctl(fildes, VIDIOC_TRY_EXT_CTRLS, &ctrls) < 0
            &&
                errno == EINVAL
            )
            {
                bad_ctrl_class:
                explain_buffer_einval_vague(sb, "data->ctrl_class");
                return;
            }

            /*
             * One of the controls may be the problem.
             */
            for (j = 0; j < arg->count; ++j)
            {
                struct v4l2_ext_control ctrl;

                /* not risking changing caller's data */
                ctrl = arg->controls[j];

                memset(&ctrls, 0, sizeof(ctrls));
                ctrls.ctrl_class = arg->ctrl_class;
                ctrls.controls = &ctrl;
                ctrls.count = 1;

                if
                (
                    ioctl(fildes, VIDIOC_TRY_EXT_CTRLS, &ctrls) < 0
                &&
                    errno == EINVAL
                )
                {
                    char            name[64];

                    /*
                     * Check the the control id.
                     */
                    ctrl = arg->controls[j];
                    memset(&ctrls, 0, sizeof(ctrl));
                    ctrls.ctrl_class = arg->ctrl_class;
                    ctrls.controls = &ctrl;
                    ctrls.count = 1;

                    if
                    (
                        ioctl(fildes, VIDIOC_G_EXT_CTRLS, &ctrls) < 0
                    &&
                        errno == EINVAL
                    )
                    {
                        snprintf(name, sizeof(name),
                            "data->controls[%d].id", j);
                    }
                    else
                    {
                        /*
                         * Must be the value.
                         */
                        snprintf(name, sizeof(name),
                            "data->controls[%d].value", j);
                    }
                    explain_buffer_einval_vague(sb, name);
                    return;
                }
            }
        }

        /* No idea. */
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


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)request;
    explain_buffer_v4l2_ext_controls(sb, data, 1, fildes);
}


const explain_iocontrol_t explain_iocontrol_vidioc_s_ext_ctrls =
{
    "VIDIOC_S_EXT_CTRLS", /* name */
    VIDIOC_S_EXT_CTRLS, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(struct v4l2_ext_controls), /* data_size */
    "struct v4l2_ext_controls *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

const explain_iocontrol_t explain_iocontrol_vidioc_try_ext_ctrls =
{
    "VIDIOC_TRY_EXT_CTRLS", /* name */
    VIDIOC_TRY_EXT_CTRLS, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(struct v4l2_ext_controls), /* data_size */
    "struct v4l2_ext_controls *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_S_EXT_CTRLS */

const explain_iocontrol_t explain_iocontrol_vidioc_s_ext_ctrls =
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

const explain_iocontrol_t explain_iocontrol_vidioc_try_ext_ctrls =
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

#endif /* VIDIOC_S_EXT_CTRLS */

/* vim: set ts=8 sw=4 et : */
