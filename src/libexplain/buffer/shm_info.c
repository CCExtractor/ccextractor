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

#include <libexplain/ac/sys/shm.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/shm_info.h>
#include <libexplain/is_efault.h>


void
explain_buffer_shm_info(explain_string_buffer_t *sb,
    const struct shm_info *data)
{
#ifdef HAVE_SYS_SHM_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ used_ids = ");
    explain_buffer_int(sb, data->used_ids);
    explain_string_buffer_puts(sb, ", shm_tot = ");
    explain_buffer_ulong(sb, data->shm_tot);
    explain_string_buffer_puts(sb, ", shm_rss = ");
    explain_buffer_ulong(sb, data->shm_rss);
    explain_string_buffer_puts(sb, ", shm_swp = ");
    explain_buffer_ulong(sb, data->shm_swp);
    if (data->swap_attempts)
    {
        explain_string_buffer_puts(sb, ", swap_attempts = ");
        explain_buffer_ulong(sb, data->swap_attempts);
    }
    if (data->swap_successes)
    {
        explain_string_buffer_puts(sb, ", swap_successes = ");
        explain_buffer_ulong(sb, data->swap_successes);
    }
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
