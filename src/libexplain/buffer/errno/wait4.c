/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/wait4.h>
#include <libexplain/buffer/errno/waitpid.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/waitpid_options.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_wait4_system_call(explain_string_buffer_t *sb,
    int errnum, int pid, int *status, int options, struct rusage *rusage)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "wait4(pid = ");
    explain_buffer_pid_t(sb, pid);
    if (pid == 0)
    {
        explain_string_buffer_puts(sb, " = process group ");
        explain_buffer_pid_t(sb, getpgrp());
    }
    else if (pid < -1)
    {
        explain_string_buffer_puts(sb, " = process group ");
        explain_buffer_pid_t(sb, -pid);
    }
    explain_string_buffer_puts(sb, ", status = ");
    explain_buffer_pointer(sb, status);
    explain_string_buffer_puts(sb, ", options = ");
    explain_buffer_waitpid_options(sb, options);
    explain_string_buffer_puts(sb, ", rusage = ");
    explain_buffer_pointer(sb, rusage);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_wait4_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int pid, int *status, int options,
    struct rusage *rusage)
{
    switch (errnum)
    {
    case EFAULT:
        if (rusage && explain_is_efault_pointer(rusage, sizeof(*rusage)))
        {
            explain_buffer_efault(sb, "rusage");
            break;
        }
        if (explain_is_efault_pointer(status, sizeof(*status)))
        {
            explain_buffer_efault(sb, "status");
            break;
        }
        break;

    default:
        explain_buffer_errno_waitpid_explanation
        (
            sb,
            errnum,
            syscall_name,
            pid,
            status,
            options
        );
        break;
    }
}


void
explain_buffer_errno_wait4(explain_string_buffer_t *sb, int errnum,
    int pid, int *status, int options, struct rusage *rusage)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_wait4_system_call
    (
        &exp.system_call_sb,
        errnum,
        pid,
        status,
        options,
        rusage
    );
    explain_buffer_errno_wait4_explanation
    (
        &exp.explanation_sb,
        errnum,
        "wait4",
        pid,
        status,
        options,
        rusage
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
