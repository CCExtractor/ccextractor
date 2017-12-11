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

#include <libexplain/ac/linux/scc.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/scc_kiss_cmd.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SCC_H

static void
explain_buffer_scc_param(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "PARAM_DATA", PARAM_DATA },
        { "PARAM_TXDELAY", PARAM_TXDELAY },
        { "PARAM_PERSIST", PARAM_PERSIST },
        { "PARAM_SLOTTIME", PARAM_SLOTTIME },
        { "PARAM_TXTAIL", PARAM_TXTAIL },
        { "PARAM_FULLDUP", PARAM_FULLDUP },
        { "PARAM_SOFTDCD", PARAM_SOFTDCD },
        { "PARAM_MUTE", PARAM_MUTE },
        { "PARAM_DTR", PARAM_DTR },
        { "PARAM_RTS", PARAM_RTS },
        { "PARAM_SPEED", PARAM_SPEED },
        { "PARAM_ENDDELAY", PARAM_ENDDELAY },
        { "PARAM_GROUP", PARAM_GROUP },
        { "PARAM_IDLE", PARAM_IDLE },
        { "PARAM_MIN", PARAM_MIN },
        { "PARAM_MAXKEY", PARAM_MAXKEY },
        { "PARAM_WAIT", PARAM_WAIT },
        { "PARAM_MAXDEFER", PARAM_MAXDEFER },
        { "PARAM_TX", PARAM_TX },
        { "PARAM_HWEVENT", PARAM_HWEVENT },
        { "PARAM_RETURN", PARAM_RETURN },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_scc_kiss_cmd(explain_string_buffer_t *sb,
    const struct scc_kiss_cmd *data, int all)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ command = ");
    explain_buffer_scc_param(sb, data->command);
    if (all)
    {
        explain_string_buffer_puts(sb, ", param = ");
        explain_buffer_uint(sb, data->param);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_scc_kiss_cmd(explain_string_buffer_t *sb,
    const struct scc_kiss_cmd *data, int all)
{
    (void)all;
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
