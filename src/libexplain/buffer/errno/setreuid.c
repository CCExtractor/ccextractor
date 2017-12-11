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

#include <libexplain/buffer/errno/setresuid.h>
#include <libexplain/buffer/errno/setreuid.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setreuid_system_call(explain_string_buffer_t *sb, int
    errnum, uid_t ruid, uid_t euid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setreuid(ruid = ");
    explain_buffer_uid(sb, ruid);
    explain_string_buffer_puts(sb, ", euid = ");
    explain_buffer_uid(sb, euid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setreuid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, uid_t ruid, uid_t euid)
{
    /*
     * Note that this isn't strictly correct.  On Linux, at least, the
     * two functions are actually asymmetric, with setresuid permitting
     * things that setreuid does not.
     */
    explain_buffer_errno_setresuid_explanation(sb, errnum, syscall_name, ruid,
        euid, -1);
}


void
explain_buffer_errno_setreuid(explain_string_buffer_t *sb, int errnum,
    uid_t ruid, uid_t euid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setreuid_system_call(&exp.system_call_sb, errnum, ruid,
        euid);
    explain_buffer_errno_setreuid_explanation(&exp.explanation_sb, errnum,
        "setreuid", ruid, euid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
