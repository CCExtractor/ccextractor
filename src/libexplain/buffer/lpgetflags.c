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
#include <libexplain/buffer/lpgetflags.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef LAVE_LINUX_LP_H

void
explain_buffer_lpgetflags(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "LP_EXIST", LP_EXIST },
        { "LP_SELEC", LP_SELEC },
        { "LP_BUSY", LP_BUSY },
        { "LP_OFFL", LP_OFFL },
        { "LP_NOPA", LP_NOPA },
        { "LP_ERR", LP_ERR },
        { "LP_ABORT", LP_ABORT },
        { "LP_CAREFUL", LP_CAREFUL },
        { "LP_ABORTOPEN", LP_ABORTOPEN },
        { "LP_TRUST_IRQ_", LP_TRUST_IRQ_ },
        { "LP_NO_REVERSE", LP_NO_REVERSE },
        { "LP_DATA_AVAIL", LP_DATA_AVAIL },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}

#else

void
explain_buffer_lpgetflags(explain_string_buffer_t *sb, int data)
{
    explain_buffer_int(sb, data);
}

#endif


void
explain_buffer_lpgetflags_star(explain_string_buffer_t *sb, const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_lpgetflags(sb, *data);
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
