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
#include <libexplain/buffer/errno/getresuid.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_getresuid_system_call(explain_string_buffer_t *sb, int
    errnum, uid_t *ruid, uid_t *euid, uid_t *suid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getresuid(ruid = ");
    explain_buffer_pointer(sb, ruid);
    explain_string_buffer_puts(sb, ", euid = ");
    explain_buffer_pointer(sb, euid);
    explain_string_buffer_puts(sb, ", suid = ");
    explain_buffer_pointer(sb, suid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getresuid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, uid_t *ruid, uid_t *euid, uid_t *suid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getresuid.html
     */
    switch (errnum)
    {
    case EFAULT:
        /*
         * None of these pointer are permitted to be NULL,
         * even though some other system calls permit this.
         */
        if (explain_is_efault_pointer(ruid, sizeof(*ruid)))
        {
            explain_buffer_efault(sb, "ruid");
            break;
        }
        if (explain_is_efault_pointer(euid, sizeof(*euid)))
        {
            explain_buffer_efault(sb, "euid");
            break;
        }
        if (explain_is_efault_pointer(suid, sizeof(*suid)))
        {
            explain_buffer_efault(sb, "suid");
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_getresuid(explain_string_buffer_t *sb, int errnum, uid_t
    *ruid, uid_t *euid, uid_t *suid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getresuid_system_call(&exp.system_call_sb, errnum,
        ruid, euid, suid);
    explain_buffer_errno_getresuid_explanation(&exp.explanation_sb, errnum,
        "getresuid", ruid, euid, suid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
