/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2012, 2013 Peter Miller
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
#include <libexplain/ac/poll.h>

#include <libexplain/output.h>
#include <libexplain/poll.h>


int
explain_poll_or_die(struct pollfd *data, int data_size, int timeout)
{
    int             result;

    result = explain_poll_on_error(data, data_size, timeout);
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


int
explain_poll_on_error(struct pollfd *data, int data_size, int timeout)
{
    int             result;

#ifdef HAVE_POLL
    result = poll(data, data_size, timeout);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_poll(hold_errno, data, data_size, timeout)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
