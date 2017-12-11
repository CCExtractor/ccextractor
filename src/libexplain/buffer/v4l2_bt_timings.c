/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/linux/videodev2.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/v4l2_bt_timings.h>
#include <libexplain/buffer/v4l2_bt_timings_interlaced.h>
#include <libexplain/buffer/v4l2_bt_timings_polarities.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H
#if defined(VIDIOC_G_DV_TIMINGS) || defined(VIDIOC_S_DV_TIMINGS)

void
explain_buffer_v4l2_bt_timings(explain_string_buffer_t *sb,
    const struct v4l2_bt_timings *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ width = ");
    explain_buffer_uint32_t(sb, data->width);
    explain_string_buffer_puts(sb, ", height = ");
    explain_buffer_uint32_t(sb, data->height);
    explain_string_buffer_puts(sb, ", interlaced = ");
    explain_buffer_v4l2_bt_timings_interlaced(sb, data->interlaced);
    explain_string_buffer_puts(sb, ", polarities = ");
    explain_buffer_v4l2_bt_timings_polarities(sb, data->polarities);
    if (data->pixelclock)
    {
        explain_string_buffer_puts(sb, ", pixelclock = ");
        explain_buffer_uint64_t(sb, data->pixelclock);
    }
    if (data->hfrontporch)
    {
        explain_string_buffer_puts(sb, ", hfrontporch = ");
        explain_buffer_uint32_t(sb, data->hfrontporch);
    }
    if (data->hsync)
    {
        explain_string_buffer_puts(sb, ", hsync = ");
        explain_buffer_uint32_t(sb, data->hsync);
    }
    if (data->hbackporch)
    {
        explain_string_buffer_puts(sb, ", hbackporch = ");
        explain_buffer_uint32_t(sb, data->hbackporch);
    }
    if (data->vfrontporch)
    {
        explain_string_buffer_puts(sb, ", vfrontporch = ");
        explain_buffer_uint32_t(sb, data->vfrontporch);
    }
    if (data->vsync)
    {
        explain_string_buffer_puts(sb, ", vsync = ");
        explain_buffer_uint32_t(sb, data->vsync);
    }
    if (data->vbackporch)
    {
        explain_string_buffer_puts(sb, ", vfrontporch = ");
        explain_buffer_uint32_t(sb, data->vbackporch);
    }
    if (data->il_vfrontporch)
    {
        explain_string_buffer_puts(sb, ", il_vfrontporch = ");
        explain_buffer_uint32_t(sb, data->il_vfrontporch);
    }
    if (data->il_vsync)
    {
        explain_string_buffer_puts(sb, ", il_vsync = ");
        explain_buffer_uint32_t(sb, data->il_vsync);
    }
    if (data->il_vbackporch)
    {
        explain_string_buffer_puts(sb, ", il_vbackporch = ");
        explain_buffer_uint32_t(sb, data->il_vbackporch);
    }
    if (!explain_uint32_array_all_zero(data->reserved, SIZEOF(data->reserved)))
    {
        explain_string_buffer_puts(sb, ", reserved = ");
        explain_buffer_uint32_array(sb, data->reserved, SIZEOF(data->reserved));
    }
    explain_string_buffer_puts(sb, " }");
}

#endif
#endif


/* vim: set ts=8 sw=4 et : */
