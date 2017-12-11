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

#include <libexplain/buffer/termio_baud.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


void
explain_buffer_termio_baud(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "B0", B0 },
        { "B50", B50 },
        { "B75", B75 },
        { "B110", B110 },
        { "B134", B134 },
        { "B150", B150 },
        { "B200", B200 },
        { "B300", B300 },
        { "B600", B600 },
        { "B1200", B1200 },
        { "B1800", B1800 },
        { "B2400", B2400 },
        { "B4800", B4800 },
        { "B9600", B9600 },
        { "B19200", B19200 },
        { "B38400", B38400 },
#ifdef BOTHER
        { "BOTHER", BOTHER },
#endif
#ifdef B57600
        { "B57600", B57600 },
#endif
#ifdef B115200
        { "B115200", B115200 },
#endif
#ifdef B230400
        { "B230400", B230400 },
#endif
#ifdef B460800
        { "B460800", B460800 },
#endif
#ifdef B500000
        { "B500000", B500000 },
#endif
#ifdef B576000
        { "B576000", B576000 },
#endif
#ifdef B921600
        { "B921600", B921600 },
#endif
#ifdef B1000000
        { "B1000000", B1000000 },
#endif
#ifdef B1152000
        { "B1152000", B1152000 },
#endif
#ifdef B1500000
        { "B1500000", B1500000 },
#endif
#ifdef B2000000
        { "B2000000", B2000000 },
#endif
#ifdef B2500000
        { "B2500000", B2500000 },
#endif
#ifdef B3000000
        { "B3000000", B3000000 },
#endif
#ifdef B3500000
        { "B3500000", B3500000 },
#endif
#ifdef B4000000
        { "B4000000", B4000000 },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
