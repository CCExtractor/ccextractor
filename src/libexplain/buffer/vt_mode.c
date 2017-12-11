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

#include <libexplain/ac/linux/vt.h>

#include <libexplain/buffer/int8.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/signal.h>
#include <libexplain/buffer/vt_mode.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VT_H


void
explain_buffer_vt_mode_mode(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VT_AUTO", VT_AUTO },
        { "VT_PROCESS", VT_PROCESS },
        { "VT_ACKACQ", VT_ACKACQ },
    };
    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_vt_mode(explain_string_buffer_t *sb,
    const struct vt_mode *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ mode = ");
    explain_buffer_vt_mode_mode(sb, data->mode);
    if (data->waitv)
    {
        explain_string_buffer_puts(sb, ", waitv = ");
        explain_buffer_int8(sb, data->waitv);
    }
    if (data->relsig)
    {
        explain_string_buffer_puts(sb, ", relsig = ");
        explain_buffer_signal(sb, data->relsig);
    }
    if (data->acqsig)
    {
        explain_string_buffer_puts(sb, ", acqsig = ");
        explain_buffer_signal(sb, data->acqsig);
    }
    if (data->frsig)
    {
        explain_string_buffer_puts(sb, ", frsig = ");
        explain_buffer_signal(sb, data->frsig);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_vt_mode(explain_string_buffer_t *sb,
    const struct vt_mode *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
