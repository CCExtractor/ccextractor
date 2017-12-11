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

#include <libexplain/ac/linux/cdrom.h>

#include <libexplain/buffer/cdrom_addr.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/int.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_CDROM_H

void
explain_buffer_cdrom_addr(explain_string_buffer_t *sb,
    const union cdrom_addr *addr, int fmt)
{
    if (explain_is_efault_pointer(addr, sizeof(*addr)))
    {
        explain_buffer_pointer(sb, addr);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    if (fmt == CDROM_LBA)
    {
        explain_string_buffer_puts(sb, "lba = ");
        explain_buffer_int(sb, addr->lba);
    }
    else
    {
        explain_string_buffer_printf
        (
            sb,
            "msf = { %d, %d, %d }",
            addr->msf.minute,
            addr->msf.second,
            addr->msf.frame
        );
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_cdrom_addr(explain_string_buffer_t *sb,
    const union cdrom_addr *addr, int fmt)
{
    (void)fmt;
    explain_buffer_pointer(sb, addr);
}

#endif


/* vim: set ts=8 sw=4 et : */
