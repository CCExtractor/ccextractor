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

#include <libexplain/ac/sys/mtio.h>

#include <libexplain/buffer/mtconfiginfo.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef MTIOCSETCONFIG

void
explain_buffer_mtconfiginfo(explain_string_buffer_t *sb,
    const struct mtconfiginfo *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_printf(sb, "{ mt_type = %ld, ", data->mt_type);
#ifdef __linux__
        explain_string_buffer_printf(sb, "ifc_type = %ld, ", data->ifc_type);
        explain_string_buffer_printf(sb, "irqnr = %u, ", data->irqnr);
        explain_string_buffer_printf(sb, "dmanr = %u, ", data->dmanr);
        explain_string_buffer_printf(sb, "port = %u, ", data->port);
        explain_string_buffer_printf(sb, "debug = %#lx, ", data->debug);
        explain_string_buffer_printf
        (
            sb,
            "have_dens = %d, ",
            data->have_dens
        );
        explain_string_buffer_printf(sb, "have_bsf = %d, ", data->have_bsf);
        explain_string_buffer_printf(sb, "have_fsr = %d, ", data->have_fsr);
        explain_string_buffer_printf(sb, "have_bsr = %d, ", data->have_bsr);
        explain_string_buffer_printf(sb, "have_eod = %d, ", data->have_eod);
        explain_string_buffer_printf
        (
            sb,
            "have_seek = %d, ",
            data->have_seek
        );
        explain_string_buffer_printf
        (
            sb,
            "have_tell = %d, ",
            data->have_tell
        );
        explain_string_buffer_printf
        (
            sb,
            "have_ras1 = %d, ",
            data->have_ras1
        );
        explain_string_buffer_printf
        (
            sb,
            "have_ras2 = %d, ",
            data->have_ras2
        );
        explain_string_buffer_printf
        (
            sb,
            "have_ras3 = %d, ",
            data->have_ras3
        );
        explain_string_buffer_printf
        (
            sb,
            "have_qfa = %d, ",
            data->have_qfa
        );
#endif
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_mtconfiginfo(explain_string_buffer_t *sb,
    const struct mtconfiginfo *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
