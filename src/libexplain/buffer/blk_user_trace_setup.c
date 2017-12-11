/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011-2013 Peter Miller
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

#include <libexplain/ac/linux/blktrace_api.h>

#include <libexplain/buffer/blk_user_trace_setup.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_BLKTRACE_API_H

static void
explain_buffer_blktrace_mask(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef BLK_TC_READ
        { "BLK_TC_READ", BLK_TC_READ },
#endif
#ifdef BLK_TC_WRITE
        { "BLK_TC_WRITE", BLK_TC_WRITE },
#endif
#ifdef BLK_TC_BARRIER
        { "BLK_TC_BARRIER", BLK_TC_BARRIER },
#endif
#ifdef BLK_TC_SYNC
    { "BLK_TC_SYNC", BLK_TC_SYNC },
#endif
#ifdef BLK_TC_QUEUE
    { "BLK_TC_QUEUE", BLK_TC_QUEUE },
#endif
#ifdef BLK_TC_REQUEUE
    { "BLK_TC_REQUEUE", BLK_TC_REQUEUE },
#endif
#ifdef BLK_TC_ISSUE
    { "BLK_TC_ISSUE", BLK_TC_ISSUE },
#endif
#ifdef BLK_TC_COMPLETE
    { "BLK_TC_COMPLETE", BLK_TC_COMPLETE },
#endif
#ifdef BLK_TC_FS
    { "BLK_TC_FS", BLK_TC_FS },
#endif
#ifdef BLK_TC_PC
    { "BLK_TC_PC", BLK_TC_PC },
#endif
#ifdef BLK_TC_NOTIFY
    { "BLK_TC_NOTIFY", BLK_TC_NOTIFY },
#endif
#ifdef BLK_TC_AHEAD
    { "BLK_TC_AHEAD", BLK_TC_AHEAD },
#endif
#ifdef BLK_TC_META
    { "BLK_TC_META", BLK_TC_META },
#endif
#ifdef BLK_TC_DISCARD
    { "BLK_TC_DISCARD", BLK_TC_DISCARD },
#endif
#ifdef BLK_TC_DRV_DATA
    { "BLK_TC_DRV_DATA", BLK_TC_DRV_DATA },
#endif
#ifdef BLK_TC_END
    { "BLK_TC_END", BLK_TC_END },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_blk_user_trace_setup(explain_string_buffer_t *sb,
    const struct blk_user_trace_setup *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ name = ");
    explain_string_buffer_puts_quoted_n(sb, data->name, sizeof(data->name));
    explain_string_buffer_puts(sb, ", act_mask = ");
    explain_buffer_blktrace_mask(sb, data->act_mask);
    explain_string_buffer_puts(sb, ", buf_size = ");
    explain_buffer_ulong(sb, data->buf_size);
    explain_string_buffer_puts(sb, ", buf_nr = ");
    explain_buffer_ulong(sb, data->buf_nr);
    explain_string_buffer_puts(sb, ", start_lba = ");
    explain_buffer_uint64_t(sb, data->start_lba);
    explain_string_buffer_puts(sb, ", end_lba = ");
    explain_buffer_uint64_t(sb, data->end_lba);
    explain_string_buffer_puts(sb, ", pid = ");
    explain_buffer_ulong(sb, data->pid);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_blk_user_trace_setup(explain_string_buffer_t *sb,
    const struct blk_user_trace_setup *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
