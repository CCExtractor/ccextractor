/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/tiocm.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#if defined(TIOCMBIC) || \
    defined(TIOCMBIS) || \
    defined(TIOCMGET) || \
    defined(TIOCMIWAIT) || \
    defined(TIOCMSET)

static const explain_parse_bits_table_t table[] =
{
    { "TIOCM_LE", TIOCM_LE },
    { "TIOCM_DTR", TIOCM_DTR },
    { "TIOCM_RTS", TIOCM_RTS },
    { "TIOCM_ST", TIOCM_ST },
    { "TIOCM_SR", TIOCM_SR },
    { "TIOCM_CTS", TIOCM_CTS },
    { "TIOCM_CAR", TIOCM_CAR },
    { "TIOCM_CD", TIOCM_CD },
    { "TIOCM_RNG", TIOCM_RNG },
    { "TIOCM_RI", TIOCM_RI },
    { "TIOCM_DSR", TIOCM_DSR },
};


void
explain_buffer_tiocm(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_tiocm_star(explain_string_buffer_t *sb, const int *value)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_tiocm(sb, *value);
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_tiocm_star(explain_string_buffer_t *sb, const int *value)
{
    explain_buffer_pointer(sb, value);
}

#endif


/* vim: set ts=8 sw=4 et : */
