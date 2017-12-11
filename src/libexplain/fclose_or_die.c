/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/fclose.h>
#include <libexplain/fflush.h>
#include <libexplain/output.h>
#include <libexplain/stream_to_fildes.h>


static int
open_for_writing(FILE *fp)
{
    int             fildes;
    int             flags;

    fildes = explain_stream_to_fildes(fp);
    flags = fcntl(fildes, F_GETFL);
    if (flags < 0)
        return 0;
    switch (flags & O_ACCMODE)
    {
    case O_WRONLY:
    case O_RDWR:
        return 1;

    default:
        break;
    }
    return 0;
}


int
explain_fclose_on_error(FILE *fp)
{
    int             result;

    if (open_for_writing(fp))
    {
        /* only flush output streams */
        result = explain_fflush_on_error(fp);
        if (result < 0)
        {
            int             hold_errno;

            /*
             * FIXME: it would be better if we could nominate a different
             * syscall_name for this error message, rather than surprising
             * the user (or developer reading the bug report) with "fflush".
             */
            hold_errno = errno;
            explain_output_error("%s", explain_errno_fflush(hold_errno, fp));
            fclose(fp);
            errno = hold_errno;
            return result;
        }
    }

    result = fclose(fp);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_fclose(hold_errno, fp));
        errno = hold_errno;
    }
    return result;
}


void
explain_fclose_or_die(FILE *fp)
{
    if (explain_fclose_on_error(fp))
    {
        explain_output_exit_failure();
    }
}


/* vim: set ts=8 sw=4 et : */
