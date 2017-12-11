/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/scc.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/scc_mem_config.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SCC_H

void
explain_buffer_scc_mem_config(explain_string_buffer_t *sb,
    const struct scc_mem_config *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ dummy = ");
    explain_buffer_uint(sb, data->dummy);
    explain_string_buffer_puts(sb, ", bufsize = ");
    explain_buffer_uint(sb, data->bufsize);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_scc_mem_config(explain_string_buffer_t *sb,
    const struct scc_mem_config *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
