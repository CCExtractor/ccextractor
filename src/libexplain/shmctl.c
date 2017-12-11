/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/shm.h>

#include <libexplain/buffer/errno/shmctl.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/shmctl.h>


const char *
explain_shmctl(int shmid, int command, struct shmid_ds *data)
{
    return explain_errno_shmctl(errno, shmid, command, data);
}


const char *
explain_errno_shmctl(int errnum, int shmid, int command, struct shmid_ds *data)
{
    explain_message_errno_shmctl(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, shmid, command, data);
    return explain_common_message_buffer;
}


void
explain_message_shmctl(char *message, int message_size, int shmid, int command,
    struct shmid_ds *data)
{
    explain_message_errno_shmctl(message, message_size, errno, shmid, command,
        data);
}


void
explain_message_errno_shmctl(char *message, int message_size, int errnum, int
    shmid, int command, struct shmid_ds *data)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_shmctl(&sb, errnum, shmid, command, data);
}


/* vim: set ts=8 sw=4 et : */
