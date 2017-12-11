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

#include <libexplain/buffer/acl.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/acl_to_text.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acl_to_text_system_call(explain_string_buffer_t *sb,
    int errnum, acl_t acl, ssize_t *len_p)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_to_text(acl = ");
    explain_buffer_acl(sb, acl);
    explain_string_buffer_puts(sb, ", len_p = ");
    explain_buffer_pointer(sb, len_p);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_acl_to_text_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, acl_t acl, ssize_t *len_p)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acl_to_text.html
     */
    (void)len_p;
    switch (errnum)
    {
    case EINVAL:
        if (!acl)
        {
            explain_buffer_is_the_null_pointer(sb, "acl");
            break;
        }
        /*
         * The argument acl is not a valid pointer to an ACL.
         * The ACL referenced by acl contains one or more improp‐
         * erly formed ACL entries, or for some other reason can‐
         * not be translated into a text form of an ACL.
         */
        if (acl_valid(acl) < 0)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the acl argument does not point to a valid ACL"
            );
            break;
        }

        explain_buffer_einval_vague(sb, "acl");
        break;

    case ENOMEM:
        /*
         * The character string to be returned requires more mem‐
         * ory than is allowed by the hardware or system-imposed
         * memory management constraints.
         */
        explain_buffer_enomem_user(sb, 0);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_to_text(explain_string_buffer_t *sb, int errnum,
    acl_t acl, ssize_t *len_p)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_to_text_system_call(&exp.system_call_sb, errnum,
        acl, len_p);
    explain_buffer_errno_acl_to_text_explanation(&exp.explanation_sb, errnum,
        "acl_to_text", acl, len_p);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
