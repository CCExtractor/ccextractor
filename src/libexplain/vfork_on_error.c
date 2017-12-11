/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/types.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/vfork.h>
#include <libexplain/output.h>


pid_t
explain_vfork_on_error(void)
{
    pid_t           result;

    /*
     * Note: we can't use vfork(2), we have to use fork(2).
     *
     * From the vfork(2) man page:
     *
     *     "The vfork() function has the same effect as fork(2), except
     *     that the behavior is undefined if the process created by
     *     vfork() either modifies any data other than a variable of
     *     type pid_t used to store the return value from vfork(), or
     *     returns from the function in which vfork() was called, or
     *     calls any other function before successfully calling _exit(2)
     *     or one of the exec(3) family of functions."
     *
     * since we can't satisfy these requirements, we must use fork(2)
     * instead.  Mind you, this only happens if they are using GCC with
     * version < 3.0.
     */
    result = fork();
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_vfork(hold_errno));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
