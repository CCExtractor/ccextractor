/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getresgid.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_getresgid_system_call(explain_string_buffer_t *sb,
    int errnum, gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getresgid(rgid = ");
    explain_buffer_pointer(sb, rgid);
    explain_string_buffer_puts(sb, ", egid = ");
    explain_buffer_pointer(sb, egid);
    explain_string_buffer_puts(sb, ", sgid = ");
    explain_buffer_pointer(sb, sgid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getresgid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getresgid.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (explain_is_efault_pointer(rgid, sizeof(*rgid)))
        {
            explain_buffer_efault(sb, "rgid");
            break;
        }
        if (explain_is_efault_pointer(egid, sizeof(*egid)))
        {
            explain_buffer_efault(sb, "egid");
            break;
        }
        if (explain_is_efault_pointer(sgid, sizeof(*sgid)))
        {
            explain_buffer_efault(sb, "sgid");
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_getresgid(explain_string_buffer_t *sb, int errnum,
    gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getresgid_system_call(&exp.system_call_sb, errnum,
        rgid, egid, sgid);
    explain_buffer_errno_getresgid_explanation(&exp.explanation_sb, errnum,
        "getresgid", rgid, egid, sgid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
