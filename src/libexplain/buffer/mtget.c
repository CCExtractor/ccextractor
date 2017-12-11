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

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/mtget.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_mtget(explain_string_buffer_t *sb,
    const struct mtget *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_puts(sb, "{ mt_type = ");
        explain_buffer_long(sb, data->mt_type);
        explain_string_buffer_puts(sb, ", mt_resid = ");
        explain_buffer_long(sb, data->mt_resid);
        explain_string_buffer_puts(sb, ", mt_dsreg = ");
        explain_buffer_long(sb, data->mt_dsreg);
#ifdef __linux__
        explain_string_buffer_puts(sb, ", mt_gstat = ");
        explain_buffer_long(sb, data->mt_gstat);
#endif
        explain_string_buffer_puts(sb, ", mt_erreg = ");
        explain_buffer_long(sb, data->mt_erreg);
        explain_string_buffer_puts(sb, ", mt_fileno = ");
        explain_buffer_long(sb, data->mt_fileno);
        explain_string_buffer_puts(sb, ", mt_blkno = ");
        explain_buffer_long(sb, data->mt_blkno);
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
