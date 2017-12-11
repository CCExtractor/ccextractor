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

#include <libexplain/ac/linux/kd.h>

#include <libexplain/buffer/kbentry.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_KD_H

void
explain_buffer_kbentry(explain_string_buffer_t *sb,
    const struct kbentry *value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "K_NORMTAB", K_NORMTAB },
        { "K_SHIFTTAB", K_SHIFTTAB },
        { "K_ALTTAB", K_ALTTAB },
        { "K_ALTSHIFTTAB", K_ALTSHIFTTAB },
    };

    explain_string_buffer_puts(sb, "{ kb_table = ");
    explain_parse_bits_print_single
    (
        sb,
        value->kb_table,
        table,
        SIZEOF(table)
    );
    explain_string_buffer_printf
    (
        sb,
        ", kb_index = %#x, kb_value = %#x }",
        value->kb_index,
        value->kb_value
    );
}

#endif /* HAVE_LINUX_KD_H */


/* vim: set ts=8 sw=4 et : */
