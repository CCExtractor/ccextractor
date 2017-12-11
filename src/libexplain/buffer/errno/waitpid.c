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
#include <libexplain/ac/signal.h>
#include <libexplain/ac/sys/wait.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/waitpid.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/no_outstanding_children.h>
#include <libexplain/buffer/note/sigchld.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/waitpid_options.h>
#include <libexplain/explanation.h>
#include <libexplain/process_exists.h>


static void
explain_buffer_errno_waitpid_system_call(explain_string_buffer_t *sb,
    int errnum, int pid, int *status, int options)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "waitpid(pid = ");
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
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_waitpid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int pid, int *status, int options)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/waitpid.html
     */
    (void)status;
    switch (errnum)
    {
    case ECHILD:
        if (pid > 0)
        {
            if (explain_process_exists(pid))
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext:  This message is use when a wait*()
                     * function is asked to wait for a process that is
                     * not a child of the process.
                     *
                     * %1$s => the name of the offending system call argument
                     */
                    i18n("the process specified by %s is not a child of "
                        "this process"),
                    "pid"
                );
            }
            else
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext:  This message is use when a wait*()
                     * function is asked to wait for a process that does
                     * not exist.
                     *
                     * %1$s => the name of the offending system call argument
                     */
                    i18n("the process specified by %s does not exist"),
                    "pid"
                );
            }
        }
        else if (pid == -1)
        {
            explain_buffer_no_outstanding_children(sb);
        }
        else
        {
            int             pgid;

            pgid = pid ? -pid : getpgrp();
            if (explain_process_exists(pid))
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used when a wait*()
                     * function was called to wait for a process group
                     * that does not have any member process that is a
                     * child of this process.
                     *
                     * %1$d => is the process group number.
                     */
                    i18n("process group %d does not have any member process "
                        "that is a child of this process"),
                    pgid
                );
            }
            else
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used when a wait*()
                     * function was called to wait for a process group
                     * that does not exist.
                     *
                     * %1$d => the process group number.
                     */
                    i18n("process group %d does not exist"),
                    pgid
                );
            }
        }
        explain_buffer_note_sigchld(sb);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "status");
        break;

    case EINTR:
        /*
         * WNOHANG was not set and an unblocked signal or a SIGCHLD was
         * caught.
         */
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        explain_buffer_einval_bits(sb, "options");
        explain_string_buffer_printf
        (
            sb,
            " (%#x)",
            options & ~(WCONTINUED | WNOHANG | WUNTRACED)
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_waitpid(explain_string_buffer_t *sb, int errnum,
    int pid, int *status, int options)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_waitpid_system_call
    (
        &exp.system_call_sb,
        errnum,
        pid,
        status,
        options
    );
    explain_buffer_errno_waitpid_explanation
    (
        &exp.explanation_sb,
        errnum,
        "waitpid",
        pid,
        status,
        options
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
