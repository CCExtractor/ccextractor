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

#include <libexplain/ac/net/ppp_defs.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/ppp_comp_stats.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef SIOCGPPPCSTATS

static void
explain_buffer_compstat(explain_string_buffer_t *sb,
    const struct compstat *data)
{
    explain_string_buffer_puts(sb, "{ unc_bytes = ");
    explain_buffer_uint32_t(sb, data->unc_bytes);
    explain_string_buffer_puts(sb, ", unc_packets = ");
    explain_buffer_uint32_t(sb, data->unc_packets);
    explain_string_buffer_puts(sb, ", comp_bytes = ");
    explain_buffer_uint32_t(sb, data->comp_bytes);
    explain_string_buffer_puts(sb, ", comp_packets = ");
    explain_buffer_uint32_t(sb, data->comp_packets);
    explain_string_buffer_puts(sb, ", inc_bytes = ");
    explain_buffer_uint32_t(sb, data->inc_bytes);
    explain_string_buffer_puts(sb, ", inc_packets = ");
    explain_buffer_uint32_t(sb, data->inc_packets);
    explain_string_buffer_puts(sb, ", in_count = ");
    explain_buffer_uint32_t(sb, data->in_count);
    explain_string_buffer_puts(sb, ", bytes_out = ");
    explain_buffer_uint32_t(sb, data->bytes_out);
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_ppp_comp_stats(explain_string_buffer_t *sb,
    const struct ppp_comp_stats *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ c = ");
    explain_buffer_compstat(sb, &data->c);
    explain_string_buffer_puts(sb, ", d = ");
    explain_buffer_compstat(sb, &data->c);
    explain_string_buffer_puts(sb, " }");
}

!@#$
#else

void
explain_buffer_ppp_comp_stats(explain_string_buffer_t *sb,
    const struct ppp_comp_stats *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
