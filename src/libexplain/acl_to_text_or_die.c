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

#include <libexplain/acl_to_text.h>
#include <libexplain/output.h>


char *
explain_acl_to_text_or_die(acl_t acl, ssize_t *len_p)
{
    char            *result;

    result = explain_acl_to_text_on_error(acl, len_p);
    if (!result)
    {
        explain_output_exit_failure();
    }
    return result;
}


char *
explain_acl_to_text_on_error(acl_t acl, ssize_t *len_p)
{
    char            *result;

#ifdef HAVE_ACL_TO_TEXT
    result = acl_to_text(acl, len_p);
#else
    errno = ENOSYS;
    result = 0;
#endif
    if (!result)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_acl_to_text(hold_errno, acl, len_p)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
