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
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/v4l2_dbg_register.h>
#include <libexplain/buffer/v4l2_register.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/vidioc_dbg_g_register.h>

#ifdef VIDIOC_DBG_G_REGISTER


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
#ifdef HAVE_V4L2_DBG_REGISTER
    explain_buffer_v4l2_dbg_register(sb, data, 0);
#else
    explain_buffer_v4l2_register(sb, data, 0);
#endif
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EPERM:
        explain_buffer_eperm_vague(sb, "ioctl VIDIOC_DBG_G_REGISTER");
        explain_buffer_dac_sys_admin(sb);
        break;

    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }

        /*
         * It could be the address is invalid, or it could be that this
         * actually means ENOTTY.  Guess the latter.
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


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
#ifdef HAVE_V4L2_DBG_REGISTER
    explain_buffer_v4l2_dbg_register(sb, data, 1);
#else
    explain_buffer_v4l2_register(sb, data, 1);
#endif
}


const explain_iocontrol_t explain_iocontrol_vidioc_dbg_g_register =
{
    "VIDIOC_DBG_G_REGISTER", /* name */
    VIDIOC_DBG_G_REGISTER, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
#ifdef HAVE_V4L2_DBG_REGISTER
    sizeof(struct v4l2_dbg_register), /* data_size */
    "struct v4l2_dbg_register *", /* data_type */
    0, /* flags */
#else
    sizeof(struct v4l2_register), /* data_size */
    "struct v4l2_register *", /* data_type */
    0, /* flags */
#endif
    __FILE__,
    __LINE__,
};

#else /* ndef VIDIOC_DBG_G_REGISTER */

const explain_iocontrol_t explain_iocontrol_vidioc_dbg_g_register =
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

#endif /* VIDIOC_DBG_G_REGISTER */

/* vim: set ts=8 sw=4 et : */
