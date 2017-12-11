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

#include <libexplain/ac/linux/kd.h>

#include <libexplain/buffer/consolefontdesc.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_KD_H

void
explain_buffer_consolefontdesc(explain_string_buffer_t *sb,
    const struct consolefontdesc *value, int extra)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
    {
        explain_buffer_pointer(sb, value);
        return;
    }
    explain_string_buffer_printf
    (
        sb,
        "{ charcount = %d, charheight = %d, chardata = ",
        value->charcount,
        value->charheight
    );
    if
    (
        extra
    &&
        (value->charcount == 256 || value->charcount == 512)
    &&
        value->charheight >= 1
    &&
        value->charheight <= 32
    &&
        explain_option_debug()
    )
    {
        size_t          size;

        size = value->charcount * value->charheight;
        if (!explain_is_efault_pointer(value->chardata, size))
        {
            explain_string_buffer_putc(sb, '{');
            explain_buffer_hexdump(sb, value->chardata, size);
            explain_string_buffer_puts(sb, " } }");
            return;
        }
    }
    explain_buffer_pointer(sb, value->chardata);
    explain_string_buffer_puts(sb, " }");
}

#endif /* HAVE_LINUX_KD_H */


/* vim: set ts=8 sw=4 et : */
