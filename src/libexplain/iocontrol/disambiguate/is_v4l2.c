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

#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/iocontrol.h>


int
explain_iocontrol_disambiguate_is_v4l2(int fildes, int request,
    const void *data)
{
#ifdef HAVE_LINUX_VIDEODEV2_H
    struct v4l2_capability dummy;

    (void)request;
    (void)data;
    return (ioctl(fildes, VIDIOC_QUERYCAP, &dummy) >= 0);
#else
    return 0;
#endif
}


/* vim: set ts=8 sw=4 et : */
