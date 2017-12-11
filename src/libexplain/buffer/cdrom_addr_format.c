/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/linux/cdrom.h>

#include <libexplain/buffer/cdrom_addr_format.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_CDROM_H

void
explain_buffer_cdrom_addr_format(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "CDROM_LBA", CDROM_LBA },
        { "CDROM_MSF", CDROM_MSF },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}

#else

void
explain_buffer_cdrom_addr_format(explain_string_buffer_t *sb, int value)
{
    explain_string_buffer_printf(sb, "%d", value);
}

#endif


/* vim: set ts=8 sw=4 et : */
