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

#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setregid.h>
#include <libexplain/buffer/errno/setresgid.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setregid_system_call(explain_string_buffer_t *sb,
    int errnum, gid_t rgid, gid_t egid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setregid(rgid = ");
    explain_buffer_gid(sb, rgid);
    explain_string_buffer_puts(sb, ", egid = ");
    explain_buffer_gid(sb, egid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setregid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, gid_t rgid, gid_t egid)
{
    explain_buffer_errno_setresgid_explanation
    (
        sb,
        errnum,
        syscall_name,
        rgid,
        egid,
        -1
    );
}


void
explain_buffer_errno_setregid(explain_string_buffer_t *sb, int errnum,
    gid_t rgid, gid_t egid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setregid_system_call(&exp.system_call_sb, errnum, rgid,
        egid);
    explain_buffer_errno_setregid_explanation(&exp.explanation_sb, errnum,
        "setregid", rgid, egid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
