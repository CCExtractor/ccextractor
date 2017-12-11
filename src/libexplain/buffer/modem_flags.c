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

#include <libexplain/ac/linux/termios.h>

#include <libexplain/buffer/modem_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef TIOCM_LE

void
explain_buffer_modem_flags(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_table_t table[] =
    {
        { "TIOCM_LE", TIOCM_LE },
        { "TIOCM_DTR", TIOCM_DTR },
        { "TIOCM_RTS", TIOCM_RTS },
        { "TIOCM_ST", TIOCM_ST },
        { "TIOCM_SR", TIOCM_SR },
        { "TIOCM_CTS", TIOCM_CTS },
        { "TIOCM_CAR", TIOCM_CAR },
        { "TIOCM_RNG", TIOCM_RNG },
        { "TIOCM_DSR", TIOCM_DSR },
        { "TIOCM_CD", TIOCM_CD },
        { "TIOCM_RI", TIOCM_RI },
#ifdef TIOCM_OUT1
        { "TIOCM_OUT1", TIOCM_OUT1 },
#endif
#ifdef TIOCM_OUT2
        { "TIOCM_OUT2", TIOCM_OUT2 },
#endif
#ifdef TIOCM_LOOP
        { "TIOCM_LOOP", TIOCM_LOOP },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#else

void
explain_buffer_modem_flags(explain_string_buffer_t *sb, int value)
{
    explain_string_buffer_printf(sb, "%d", value);
}

#endif


/* vim: set ts=8 sw=4 et : */
