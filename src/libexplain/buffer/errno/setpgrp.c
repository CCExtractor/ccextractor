/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/errno/setpgid.h>
#include <libexplain/buffer/errno/setpgrp.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setpgrp_system_call(explain_string_buffer_t *sb,
    int errnum, pid_t pid, pid_t pgid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setpgrp(");
#if SETPGRP_NARGS == 0
    if (pid || pgid)
    {
#endif
        explain_string_buffer_puts(sb, "pid = ");
        explain_buffer_pid_t(sb, pid);
        explain_string_buffer_puts(sb, ", pgid = ");
        explain_buffer_pid_t(sb, pgid);
#if SETPGRP_NARGS == 0
    }
#endif
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setpgrp_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, pid_t pid, pid_t pgid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setpgrp.html
     */
    explain_buffer_errno_setpgid_explanation
    (
        sb,
        errnum,
        syscall_name,
        pid,
        pgid
    );
}


void
explain_buffer_errno_setpgrp(explain_string_buffer_t *sb, int errnum, pid_t pid,
    pid_t pgid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setpgrp_system_call(&exp.system_call_sb, errnum, pid,
        pgid);
    explain_buffer_errno_setpgrp_explanation(&exp.explanation_sb, errnum,
        "setpgrp", pid, pgid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
