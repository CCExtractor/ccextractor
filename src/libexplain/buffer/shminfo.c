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

#include <libexplain/ac/sys/shm.h>

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/shminfo.h>
#include <libexplain/is_efault.h>


void
explain_buffer_shminfo(explain_string_buffer_t *sb,
    const struct shminfo *data)
{
#ifdef HAVE_SYS_SHM_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ shmmax = ");
    explain_buffer_ulong(sb, data->shmmax);
    explain_string_buffer_puts(sb, ", shmmin = ");
    explain_buffer_ulong(sb, data->shmmin);
    explain_string_buffer_puts(sb, ", shmmni = ");
    explain_buffer_ulong(sb, data->shmmni);
    explain_string_buffer_puts(sb, ", shmseg = ");
    explain_buffer_ulong(sb, data->shmseg);
    explain_string_buffer_puts(sb, ", shmall = ");
    explain_buffer_ulong(sb, data->shmall);
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
