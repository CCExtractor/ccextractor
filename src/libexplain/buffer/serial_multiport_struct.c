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

#include <libexplain/ac/linux/serial.h>
#include <libexplain/ac/termios.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/serial_multiport_struct.h>
#include <libexplain/is_efault.h>


void
explain_buffer_serial_multiport_struct(explain_string_buffer_t *sb,
    const struct serial_multiport_struct *value)
{
#ifdef TIOCSERSETMULTI
    if (explain_is_efault_pointer(value, sizeof(*data)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_printf
        (
            sb,
            "{ "
                "irq = %d, "
                "port1 = %d, "
                "mask1 = %u, "
                "match1 = %u, "
                "port2 = %d, "
                "mask2 = %u, "
                "match2 = %u, "
                "port3 = %d, "
                "mask3 = %u, "
                "match3 = %u, "
                "port4 = %d, "
                "mask4 = %u, "
                "match4 = %u, "
                "port_monitor = %d "
            "}",
            value->irq,
            value->port1,
            value->mask1,
            value->match1,
            value->port2,
            value->mask2,
            value->match2,
            value->port3,
            value->mask3,
            value->match3,
            value->port4,
            value->mask4,
            value->match4,
            value->port_monitor
        );
    }
#else
    explain_buffer_pointer(sb, value);
#endif
}


/* vim: set ts=8 sw=4 et : */
