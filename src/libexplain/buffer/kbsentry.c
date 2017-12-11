/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/kd.h>

#include <libexplain/buffer/kbsentry.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_KD_H

void
explain_buffer_kbsentry(explain_string_buffer_t *sb,
    const struct kbsentry *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }
    explain_string_buffer_printf(sb, "{ kb_func = %u", data->kb_func);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", kb_string = ");
        explain_string_buffer_putsu_quoted(sb, data->kb_string);
    }
    explain_string_buffer_puts(sb, " }");
}

#endif /* HAVE_LINUX_KD_H */


/* vim: set ts=8 sw=4 et : */
