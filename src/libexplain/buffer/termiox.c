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

#include <libexplain/ac/linux/termios.h>

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/termiox.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#if defined(HAVE_LINUX_TERMIOS_H) && defined(NFF)

void
explain_buffer_termiox(explain_string_buffer_t *sb,
    const struct termiox *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    /* this looks to be almost entirely unsupported in Linux */
    explain_string_buffer_puts(sb, "{ ");
    explain_string_buffer_puts(sb, ", x_hflag = ");
    explain_string_buffer_printf(sb, "%#X", data->x_hflag);
    explain_string_buffer_puts(sb, ", x_cflag = ");
    explain_string_buffer_printf(sb, "%#X", data->x_cflag);
    explain_string_buffer_puts(sb, ", x_rflag = ");
    explain_buffer_hexdump(sb, data->x_rflag, sizeof(data->x_rflag));
    explain_string_buffer_puts(sb, ", x_sflag = ");
    explain_string_buffer_printf(sb, "%#X", data->x_sflag);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_termiox(explain_string_buffer_t *sb,
    const struct termiox *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
