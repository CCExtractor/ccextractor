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
#include <libexplain/ac/unistd.h>

#include <libexplain/getdomainname.h>
#include <libexplain/output.h>


int
explain_getdomainname_on_error(char *data, size_t data_size)
{
    int             result;

#ifdef HAVE_GETDOMAINNAME
    result = getdomainname(data, data_size);
#else
    errno = ENOSYS;
    result = -1;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_getdomainname(hold_errno,
            data, data_size));
        errno = hold_errno;
    }
    return result;
}


void
explain_getdomainname_or_die(char *data, size_t data_size)
{
    if (explain_getdomainname_on_error(data, data_size) < 0)
    {
        explain_output_exit_failure();
    }
}


/* vim: set ts=8 sw=4 et : */
