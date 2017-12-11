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
#include <libexplain/buffer/scc_modem.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SCC_H

static void
explain_buffer_clock_source(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "CLK_DPLL", CLK_DPLL },
        { "CLK_EXTERNAL", CLK_EXTERNAL },
        { "CLK_DIVIDER", CLK_DIVIDER },
        { "CLK_BRG ", CLK_BRG  },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_scc_modem(explain_string_buffer_t *sb,
    const struct scc_modem *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ speed = ");
    explain_buffer_int(sb, data->speed);
    explain_string_buffer_puts(sb, ", clocksrc = ");
    explain_buffer_clock_source(sb, data->clocksrc);
    explain_string_buffer_puts(sb, ", nrz = ");
    explain_buffer_int(sb, data->nrz);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_scc_modem(explain_string_buffer_t *sb,
    const struct scc_modem *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
