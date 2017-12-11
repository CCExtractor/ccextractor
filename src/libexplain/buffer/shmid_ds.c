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

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/ipc_perm.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/shmid_ds.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/time_t.h>
#include <libexplain/is_efault.h>


void
explain_buffer_shmid_ds(explain_string_buffer_t *sb,
    const struct shmid_ds *data, int extra)
{
#ifdef HAVE_SYS_SHM_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ shm_perm = ");
    explain_buffer_ipc_perm(sb, &data->shm_perm, extra);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", shm_segsz = ");
        explain_buffer_size_t(sb, data->shm_segsz);
        if (data->shm_atime)
        {
            explain_string_buffer_puts(sb, ", shm_atime = ");
            explain_buffer_time_t(sb, data->shm_atime);
        }
        if (data->shm_dtime)
        {
            explain_string_buffer_puts(sb, ", shm_dtime = ");
            explain_buffer_time_t(sb, data->shm_dtime);
        }
        if (data->shm_ctime)
        {
            explain_string_buffer_puts(sb, ", shm_ctime = ");
            explain_buffer_time_t(sb, data->shm_ctime);
        }
        explain_string_buffer_puts(sb, ", shm_cpid = ");
        explain_buffer_pid_t(sb, data->shm_cpid);
        explain_string_buffer_puts(sb, ", shm_lpid = ");
        explain_buffer_pid_t(sb, data->shm_lpid);
        explain_string_buffer_puts(sb, ", shm_nattach = ");
        explain_buffer_int(sb, data->shm_nattch);
    }
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
