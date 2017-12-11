/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/acl/libacl.h>
#include <libexplain/ac/errno.h>

#include <libexplain/buffer/acl.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_acl(explain_string_buffer_t *sb, acl_t acl)
{
    const char      *text = 0;
    int             errno_hold;

    if (acl == (acl_t)NULL)
    {
        explain_string_buffer_puts(sb, "NULL");
        return;
    }

    /*
     * The acl_to_any_text and the acl_to_text functions will
     * frequently clobber errno, because it uses numerous system
     * calls (e.g. looking up user names and group names).
     */
    errno_hold = errno;

    if (explain_is_efault_pointer(acl, sizeof(void *)))
    {
        barf:
        explain_buffer_pointer(sb, acl);
        errno = errno_hold;
        return;
    }
    if (acl_valid(acl) < 0)
        goto barf;

#ifdef HAVE_ACL_TO_ANY_TEXT
    {
        int options = TEXT_ABBREVIATE;
        text = acl_to_any_text(acl, NULL, ' ', options);
    }
#else
    {
        ssize_t len = 0;
        text = acl_to_text(acl, &len);
    }
#endif
    if (!text)
        goto barf;
    explain_string_buffer_puts_quoted(sb, text);
    acl_free((void *)text);
    errno = errno_hold;
}


/* vim: set ts=8 sw=4 et : */
