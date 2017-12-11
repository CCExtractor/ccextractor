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

#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/line_discipline.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


#ifdef N_TTY

void
explain_buffer_line_discipline(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "N_TTY", N_TTY },
        { "N_SLIP", N_SLIP },
        { "N_MOUSE", N_MOUSE },
        { "N_PPP", N_PPP },
        { "N_STRIP", N_STRIP },
        { "N_AX25", N_AX25 },
        { "N_X25", N_X25 },
        { "N_6PACK", N_6PACK },
        { "N_MASC", N_MASC },
        { "N_R3964", N_R3964 },
        { "N_PROFIBUS_FDL", N_PROFIBUS_FDL },
        { "N_IRDA", N_IRDA },
        { "N_SMSBLOCK", N_SMSBLOCK },
        { "N_HDLC", N_HDLC },
        { "N_SYNC_PPP", N_SYNC_PPP },
        { "N_HCI", N_HCI },
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}

#else

void
explain_buffer_line_discipline(explain_string_buffer_t *sb, int data)
{
    explain_string_buffer_printf(sb, "%d", data);
}

#endif


void
explain_buffer_line_discipline_star(explain_string_buffer_t *sb,
    const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_line_discipline(sb, *data);
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
