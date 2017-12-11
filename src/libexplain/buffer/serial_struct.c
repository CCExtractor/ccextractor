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
#include <libexplain/buffer/serial_struct.h>
#include <libexplain/is_efault.h>


void
explain_buffer_serial_struct(explain_string_buffer_t *sb,
    const struct serial_struct *value)
{
#ifdef TIOCSSERIAL
    if (explain_is_efault_pointer(value, sizeof(*data)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_printf
        (
            sb,
            "{ "
                "type = %d, "
                "line = %d, "
                "port = %u, "
                "irq = %d, "
                "flags = %d, "
                "xmit_fifo_size = %d, "
                "custom_divisor = %d, "
                "baud_base = %d, "
                "close_delay = %u, "
                "io_type = %i, "
                "reserved_char = { %d }, "
                "hub6 = %d, "
                "closing_wait = %u, "
                "closing_wait2 = %u, "
                "iomem_base = %p, "
                "iomem_reg_shift = %u, "
                "port_high = %u, "
                "iomap_base = %lu "
            "}",
            value->type,
            value->line,
            value->port,
            value->irq,
            value->flags,
            value->xmit_fifo_size,
            value->custom_divisor,
            value->baud_base,
            value->close_delay,
            value->io_type,
            value->reserved_char[0],
            value->hub6,
            value->closing_wait,
            value->closing_wait2,
            value->iomem_base,
            value->iomem_reg_shift,
            value->port_high,
            value->iomap_base
        );
    }
#else
    explain_buffer_pointer(sb, value);
#endif
}


/* vim: set ts=8 sw=4 et : */
