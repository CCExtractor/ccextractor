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

#include <libexplain/acl_from_text.h>
#include <libexplain/output.h>


acl_t
explain_acl_from_text_or_die(const char *text)
{
    acl_t           result;

    result = explain_acl_from_text_on_error(text);
    if (result == (acl_t)NULL)
    {
        explain_output_exit_failure();
    }
    return result;
}


acl_t
explain_acl_from_text_on_error(const char *text)
{
    acl_t           result;
    int             hold_errno = errno;

#ifdef HAVE_ACL_FROM_TEXT
    result = acl_from_text(text);
    if (result == (acl_t)NULL)
        errno = EINVAL;
#else
    errno = ENOSYS;
    result = NULL;
#endif
    if (result == (acl_t)NULL)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_acl_from_text(hold_errno, text)
        );
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
