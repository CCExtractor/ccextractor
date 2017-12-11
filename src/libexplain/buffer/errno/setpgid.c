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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/errno.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setpgid.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/explanation.h>
#include <libexplain/process_exists.h>


static void
explain_buffer_errno_setpgid_system_call(explain_string_buffer_t *sb,
    int errnum, pid_t pid, pid_t pgid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setpgid(pid = ");
    explain_buffer_pid_t(sb, pid);
    explain_string_buffer_puts(sb, ", pgid = ");
    explain_buffer_pid_t(sb, pgid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setpgid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, pid_t pid, pid_t pgid)
{
    pid_t           me;

    me = getpid();
    assert(me >= 1);
    if (!pid)
        pid = me;
    if (!pgid)
        pgid = pid;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setpgid.html
     */
    switch (errnum)
    {
    case EACCES:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an EACCES
             * error reported by the setpgid(2) system call, in the case where
             * an attempt was made to change the process group ID of one of
             * the children of the calling process and the child had already
             * performed an execve(2).
             */
            i18n("an attempt was made to change the process group ID of "
                "one of the children of the calling process and the "
                "child had already performed an execve(2)")
        );
        break;

    case EINVAL:
        if (pid < 0)
        {
            pid_too_small:
            explain_buffer_einval_too_small(sb, "pid", pid);
            break;
        }
        if (!explain_process_exists(pid))
        {
            no_such_pid:
            explain_buffer_esrch(sb, "pid", pid);
            break;
        }
        if (pgid < 0)
        {
            pgid_too_small:
            explain_buffer_einval_too_small(sb, "pgid", pgid);
            break;
        }
        if (!explain_process_exists(pgid))
        {
            no_such_pgid:
            explain_buffer_esrch(sb, "pgid", pgid);
            break;
        }
        goto generic;

    case EPERM:
        {
            pid_t           sid_of_me;
            pid_t           sid_of_pid;
            pid_t           sid_of_pgid;

            /*
             * NOTE: while pgid values are usually process IDs, they
             * don't have to be.  They can be zero, as is the case for
             * many processes spawned by the kernel itself.
             */

            sid_of_me = getsid(me);
            if (sid_of_me < 0)
                goto barf;

            if (pid < 0)
                goto pid_too_small;
            sid_of_pid = getsid(pid);
            if (sid_of_pid < 0)
            {
                if (errno == ESRCH)
                    goto no_such_pid;
                goto barf;
            }

            if (pgid < 0)
                goto pgid_too_small;
            sid_of_pgid = getsid(pgid);
            if (sid_of_pgid < 0)
            {
                if (errno == ESRCH)
                    goto no_such_pgid;
                goto barf;
            }

            /*
             * You are not allowed to move a process into a process group in a
             * different session.
             */
            if (sid_of_pid != sid_of_pgid)
            {
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext: This error message is used to explain an
                     * ESRCH error reported by a setpgid system call, in the
                     * case where an attempt was made to move a process into a
                     * process group in a different session.
                     */
                    i18n("an attempt was made to move a process into a process "
                        "group in a different session")
                );
                break;
            }

            /*
             * You are not allowed to change the process group ID of a
             * session leader.
             *
             * You can tell a process is a session group leader, because
             * the sid and the pid are the same.
             */
            if (pid == sid_of_pid && pgid != pid)
            {
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext: This error message is used to explain an ESRCH
                     * error reported by a setpgid system call, in the case
                     * where an attempt was made to change the process group ID
                     * of a session leader
                     */
                    i18n("an attempt was made to change the process group ID "
                        "of a session leader")
                );
                break;
            }

            /* we have no way to tell */
            barf:
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This error message is used to explain an ESRCH
                 * error reported by a setpgid system call, in the case where an
                 * attempt was made to move a process into a process group in a
                 * different session, or to change the process group ID of one
                 * of the children of the calling process and the child was in
                 * a different session, or to change the process group ID of a
                 * session leader
                 */
                i18n("an attempt was made to move a process into a process "
                    "group in a different session, or to change the process "
                    "group ID of one of the children of the calling process "
                    "and the child was in a different session, or to change "
                    "the process group ID of a session leader")
            );
        }
        break;

    case ESRCH:
        if (pid < 0)
            goto pid_too_small;
        if (!explain_process_exists(pid))
            goto no_such_pid;
        if (pgid < 0)
            goto pgid_too_small;
        if (!explain_process_exists(pgid))
            goto no_such_pgid;

        if (pid != me)
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This error message is used to explain an ESRCH
                 * error reported by the setpgid system call, in the case where
                 * pid is not the calling process and not a child of the calling
                 * process.
                 */
                i18n("pid is not the calling process and not a child of "
                    "the calling process")
            );
            break;
        }
        goto generic;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setpgid(explain_string_buffer_t *sb, int errnum, pid_t pid,
    pid_t pgid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setpgid_system_call(&exp.system_call_sb, errnum, pid,
        pgid);
    explain_buffer_errno_setpgid_explanation(&exp.explanation_sb, errnum,
        "setpgid", pid, pgid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
