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

#include <libexplain/ac/linux/fd.h>

#include <libexplain/buffer/floppy_raw_cmd.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_FD_H

static void
explain_buffer_floppy_raw_cmd_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "FD_RAW_READ", FD_RAW_READ },
        { "FD_RAW_WRITE", FD_RAW_WRITE },
        { "FD_RAW_NO_MOTOR", FD_RAW_NO_MOTOR },
        { "FD_RAW_DISK_CHANGE", FD_RAW_DISK_CHANGE },
        { "FD_RAW_INTR", FD_RAW_INTR },
        { "FD_RAW_SPIN", FD_RAW_SPIN },
        { "FD_RAW_NO_MOTOR_AFTER", FD_RAW_NO_MOTOR_AFTER },
        { "FD_RAW_NEED_DISK", FD_RAW_NEED_DISK },
        { "FD_RAW_NEED_SEEK", FD_RAW_NEED_SEEK },
        { "FD_RAW_MORE", FD_RAW_MORE },
        { "FD_RAW_STOP_IF_FAILURE", FD_RAW_STOP_IF_FAILURE },
        { "FD_RAW_STOP_IF_SUCCESS", FD_RAW_STOP_IF_SUCCESS },
        { "FD_RAW_SOFTFAILURE", FD_RAW_SOFTFAILURE },
        { "FD_RAW_FAILURE", FD_RAW_FAILURE },
        { "FD_RAW_HARDFAILURE", FD_RAW_HARDFAILURE },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_floppy_raw_cmd(explain_string_buffer_t *sb,
    const struct floppy_raw_cmd *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ flags = ");
    explain_buffer_floppy_raw_cmd_flags(sb, data->flags);
    if (data->data)
    {
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_pointer(sb, data->data);
    }
#if 0
    explain_string_buffer_puts(sb, ", kernel_data = ");
    explain_buffer_pointer(sb, data->kernel_data);
    explain_string_buffer_puts(sb, ", next = ");
    explain_buffer_pointer(sb, data->next);
#endif
    if (data->length)
    {
        explain_string_buffer_puts(sb, ", length = ");
        explain_buffer_long(sb, data->length);
    }
    if (data->phys_length)
    {
        explain_string_buffer_puts(sb, ", phys_length = ");
        explain_buffer_long(sb, data->phys_length);
    }
    if (data->buffer_length)
    {
        explain_string_buffer_puts(sb, ", buffer_length = ");
        explain_buffer_int(sb, data->buffer_length);
    }
    if (data->rate)
    {
        explain_string_buffer_puts(sb, ", rate = ");
        explain_buffer_int(sb, data->rate);
    }
    if (data->cmd_count)
    {
        int             n;

        explain_string_buffer_puts(sb, ", cmd_count = ");
        explain_buffer_int(sb, data->cmd_count);

        n = data->cmd_count;
        if (n > (int)sizeof(data->cmd))
            n = sizeof(data->cmd);
        if (n > 0)
        {
            explain_string_buffer_puts(sb, ", cmd = ");
            explain_buffer_hexdump(sb, data->cmd, n);
        }
    }

    if (extra && data->reply_count)
    {
        int             n;

        explain_string_buffer_puts(sb, ", reply_count = ");
        explain_buffer_int(sb, data->reply_count);

        n = data->reply_count;
        if (n > (int)sizeof(data->reply))
            n = sizeof(data->reply);
        if (n > 0)
        {
            explain_string_buffer_puts(sb, ", reply = ");
            explain_buffer_hexdump(sb, data->reply, n);
        }
    }

    if (data->track)
    {
        explain_string_buffer_puts(sb, ", track = ");
        explain_buffer_int(sb, data->track);
    }
    if (extra && data->resultcode)
    {
        explain_string_buffer_puts(sb, ", resultcode = ");
        explain_buffer_int(sb, data->resultcode);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_raw_cmd(explain_string_buffer_t *sb,
    const struct floppy_raw_cmd *data, int extra)
{
    (void)extra;
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
