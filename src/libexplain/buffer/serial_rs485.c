/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/serial_rs485.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_STRUCT_SERIAL_RS485

static void
explain_buffer_serial_rs485_flags(explain_string_buffer_t *sb, uint32_t value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef SER_RS485_ENABLED
        { "SER_RS485_ENABLED", SER_RS485_ENABLED },
#endif
#ifdef SER_RS485_RTS_ON_SEND
        { "SER_RS485_RTS_ON_SEND", SER_RS485_RTS_ON_SEND },
#endif
#ifdef SER_RS485_RTS_AFTER_SEND
        { "SER_RS485_RTS_AFTER_SEND", SER_RS485_RTS_AFTER_SEND },
#endif
#ifdef SER_RS485_RTS_BEFORE_SEND
        { "SER_RS485_RTS_BEFORE_SEND", SER_RS485_RTS_BEFORE_SEND },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_serial_rs485(explain_string_buffer_t *sb,
    const struct serial_rs485 *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ flags = ");
    explain_buffer_serial_rs485_flags(sb, data->flags);
    if (data->delay_rts_before_send)
    {
        explain_string_buffer_puts(sb, ", delay_rts_before_send = ");
        explain_buffer_uint32_t(sb, data->delay_rts_before_send);
    }
#ifdef HAVE_STRUCT_SERIAL_RS485_delay_rts_after_send
    if (data->delay_rts_after_send)
    {
        explain_string_buffer_puts(sb, ", delay_rts_after_send = ");
        explain_buffer_uint32_t(sb, data->delay_rts_after_send);
    }
#endif
    if (!explain_uint32_array_all_zero(data->padding, SIZEOF(data->padding)))
    {
        explain_string_buffer_puts(sb, ", padding = ");
        explain_buffer_uint32_array(sb, data->padding, SIZEOF(data->padding));
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_serial_rs485(explain_string_buffer_t *sb,
    const struct serial_rs485 *data)
{
    explain_buffer_pointer(sb, data);
}

#endif
/* vim: set ts=8 sw=4 et : */
