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

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/line_discipline.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/termio_cc.h>
#include <libexplain/buffer/termio_cflag.h>
#include <libexplain/buffer/termio_iflag.h>
#include <libexplain/buffer/termio_lflag.h>
#include <libexplain/buffer/termio_oflag.h>
#include <libexplain/buffer/termios2.h>
#include <libexplain/is_efault.h>


#if defined(HAVE_LINUX_TERMIOS_H) && defined(TCGETS2)

void
explain_buffer_termios2(explain_string_buffer_t *sb,
    const struct termios2 *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ c_iflag = ");
    explain_buffer_termio_iflag(sb, data->c_iflag);
    explain_string_buffer_puts(sb, ", c_oflag = ");
    explain_buffer_termio_oflag(sb, data->c_oflag);
    explain_string_buffer_puts(sb, ", c_cflag = ");
    explain_buffer_termio_cflag(sb, data->c_cflag);
    explain_string_buffer_puts(sb, ", c_lflag = ");
    explain_buffer_termio_lflag(sb, data->c_lflag);
    explain_string_buffer_puts(sb, ", c_line = ");
    explain_buffer_line_discipline(sb, data->c_line);
    explain_string_buffer_puts(sb, ", c_cc = ");
    explain_buffer_termio_cc(sb, data->c_cc, sizeof(data->c_cc));
    explain_string_buffer_puts(sb, ", c_ispeed = ");
    explain_buffer_int(sb, data->c_ispeed);
    explain_string_buffer_puts(sb, ", c_ospeed = ");
    explain_buffer_int(sb, data->c_ospeed);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_termios2(explain_string_buffer_t *sb,
    const struct termios2 *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
