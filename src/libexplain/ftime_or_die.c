/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/time.h>
#include <libexplain/ac/sys/timeb.h>

#include <libexplain/ftime.h>
#include <libexplain/output.h>
#include <libexplain/is_efault.h>


void
explain_ftime_or_die(struct timeb *tp)
{
    if (explain_ftime_on_error(tp) < 0)
    {
        explain_output_exit_failure();
    }
}


#ifndef HAVE_FTIME

int
ftime(timebuf)
     struct timeb *timebuf;
{
    struct timeval tv;
    struct timezone tz;

    if (explain_is_efault_pointer(timebuf, sizeof(*timebuf)))
    {
        errno = EFAULT;
        return -1;
    }
    if (gettimeofday(&tv, &tz) < 0)
        return -1;
    timebuf->time = tv.tv_sec;
    timebuf->millitm = tv.tv_usec / 1000;
    timebuf->timezone = tz.tz_minuteswest;
    timebuf->dstflag = tz.tz_dsttime;
    return 0;
}

#endif


int
explain_ftime_on_error(struct timeb *tp)
{
    int             result;

    result = ftime(tp);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_ftime(hold_errno, tp));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
