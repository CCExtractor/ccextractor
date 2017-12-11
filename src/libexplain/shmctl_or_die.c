/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011-2013 Peter Miller
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

#include <libexplain/output.h>
#include <libexplain/shmctl.h>


#ifndef HAVE_SHMCTL

static int
shmctl(int shmid, int command, struct shmid_ds *data)
{
    (void)shmid;
    (void)command;
    (void)data;
    errno = ENOSYS;
    return -1;
}

#endif

int
explain_shmctl_or_die(int shmid, int command, struct shmid_ds *data)
{
    int             result;

    result = explain_shmctl_on_error(shmid, command, data);
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


int
explain_shmctl_on_error(int shmid, int command, struct shmid_ds *data)
{
    int             result;

    result = shmctl(shmid, command, data);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_shmctl(hold_errno, shmid, command, data)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
