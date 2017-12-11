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
#include <libexplain/buffer/cdrom_addr_format.h>
#include <libexplain/buffer/cdrom_subchnl.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_CDROM_H

void
explain_buffer_cdrom_subchnl(explain_string_buffer_t *sb,
    const struct cdrom_subchnl *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ cdsc_format = ");
    explain_buffer_cdrom_addr_format(sb, data->cdsc_format);
    explain_string_buffer_puts(sb, ", cdsc_audiostatus = ");
    explain_buffer_int(sb, data->cdsc_audiostatus);
    explain_string_buffer_puts(sb, ", cdsc_adr = ");
    explain_buffer_int(sb, data->cdsc_adr);
    explain_string_buffer_puts(sb, ", cdsc_ctrl = ");
    explain_buffer_int(sb, data->cdsc_ctrl);
    explain_string_buffer_puts(sb, ", cdsc_trk = ");
    explain_buffer_int(sb, data->cdsc_trk);
    explain_string_buffer_puts(sb, ", cdsc_ind = ");
    explain_buffer_int(sb, data->cdsc_ind);
    explain_string_buffer_puts(sb, ", cdsc_absaddr = ");
    explain_buffer_cdrom_addr(sb, &data->cdsc_absaddr, data->cdsc_format);
    explain_string_buffer_puts(sb, ", cdsc_reladdr = ");
    explain_buffer_cdrom_addr(sb, &data->cdsc_reladdr, data->cdsc_format);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_cdrom_subchnl(explain_string_buffer_t *sb,
    const struct cdrom_subchnl *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
