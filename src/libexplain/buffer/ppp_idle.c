/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/filter.h>
#include <libexplain/ac/net/ppp_defs.h>

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/ppp_idle.h>
#include <libexplain/is_efault.h>


void
explain_buffer_ppp_idle(explain_string_buffer_t *sb,
    const struct ppp_idle *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ xmit_idle = ");
    explain_buffer_long(sb, data->xmit_idle);
    explain_string_buffer_puts(sb, ", recv_idle = ");
    explain_buffer_long(sb, data->recv_idle);
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
