/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
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
#include <libexplain/ac/sys/acl.h>

#include <libexplain/acl_set_file.h>
#include <libexplain/output.h>


void
explain_acl_set_file_or_die(const char *pathname, acl_type_t type, acl_t acl)
{
    if (explain_acl_set_file_on_error(pathname, type, acl) < 0)
    {
        explain_output_exit_failure();
    }
}


int
explain_acl_set_file_on_error(const char *pathname, acl_type_t type, acl_t acl)
{
    int             result;

#ifdef HAVE_ACL_SET_FILE
    result = acl_set_file(pathname, type, acl);
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
            explain_errno_acl_set_file(hold_errno, pathname, type, acl)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
