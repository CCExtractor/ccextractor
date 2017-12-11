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

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_priority.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_priority(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_PRIORITY_UNSET", V4L2_PRIORITY_UNSET },
        { "V4L2_PRIORITY_BACKGROUND", V4L2_PRIORITY_BACKGROUND },
        { "V4L2_PRIORITY_INTERACTIVE", V4L2_PRIORITY_INTERACTIVE },
        { "V4L2_PRIORITY_RECORD", V4L2_PRIORITY_RECORD },
        { "V4L2_PRIORITY_DEFAULT", V4L2_PRIORITY_DEFAULT },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_v4l2_priority_ptr(explain_string_buffer_t *sb, const void *data)
{
    const enum v4l2_priority *p;

    p = data;
    if (explain_is_efault_pointer(p, sizeof(*p)))
    {
        explain_buffer_pointer(sb, data);
    }
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_v4l2_priority(sb, *p);
        explain_string_buffer_puts(sb, " }");
    }
}

#endif

/* vim: set ts=8 sw=4 et : */
