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

#include <libexplain/acl_grammar.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/acl_from_text.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acl_from_text_system_call(explain_string_buffer_t *sb, int
    errnum, const char *text)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_from_text(text = ");
    explain_string_buffer_puts_quoted(sb, text);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_acl_from_text_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *text)
{
    switch (errnum)
    {
    case EINVAL:
        /*
         * The argument text cannot be translated into an ACL.
         */
        if (!text)
        {
            explain_buffer_is_the_null_pointer(sb, text);
            break;
        }
        explain_string_buffer_puts(sb, explain_acl_parse(text));
        break;

    case ENOMEM:
        /*
         * The acl_t to be returned requires more memory than is
         * allowed by the hardware or system-imposed memory management
         * constraints.
         */
        explain_buffer_enomem_user(sb, 0);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_from_text(explain_string_buffer_t *sb, int errnum,
    const char *text)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_from_text_system_call
    (
        &exp.system_call_sb,
        errnum,
        text
    );
    explain_buffer_errno_acl_from_text_explanation
    (
        &exp.explanation_sb,
        errnum,
        "acl_from_text",
        text
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
