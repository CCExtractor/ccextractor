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

#include <libexplain/ac/linux/lp.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/lp_stats.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef LP_STATS

void
explain_buffer_lp_stats(explain_string_buffer_t *sb,
    const struct lp_stats *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ chars = ");
    explain_buffer_ulong(sb, data->chars);
    explain_string_buffer_puts(sb, ", sleeps = ");
    explain_buffer_ulong(sb, data->sleeps);
    explain_string_buffer_puts(sb, ", maxrun = ");
    explain_buffer_uint(sb, data->maxrun);
    explain_string_buffer_puts(sb, ", maxwait = ");
    explain_buffer_uint(sb, data->maxwait);
    explain_string_buffer_puts(sb, ", meanwait = ");
    explain_buffer_uint(sb, data->meanwait);
    explain_string_buffer_puts(sb, ", mdev = ");
    explain_buffer_uint(sb, data->mdev);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_lp_stats(explain_string_buffer_t *sb,
    const struct lp_stats *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
