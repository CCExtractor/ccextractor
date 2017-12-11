/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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
#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setpriority.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/prio_which.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>
#include <libexplain/uid_from_pid.h>


static void
explain_buffer_errno_setpriority_system_call(explain_string_buffer_t *sb, int
    errnum, int which, int who, int prio)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setpriority(which = ");
    explain_buffer_prio_which(sb, which);
    explain_string_buffer_puts(sb, ", who = ");
    switch (which)
    {
    case PRIO_PROCESS:
    case PRIO_PGRP:
        explain_buffer_pid_t(sb, who);
        break;

    case PRIO_USER:
        explain_buffer_uid(sb, who);
        break;

    default:
        explain_buffer_int(sb, who);
        break;
    }
    explain_string_buffer_puts(sb, ", prio = ");
    explain_buffer_int(sb, prio);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setpriority_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int which, int who, int prio)
{
    (void)prio;
    switch (errnum)
    {
    case EINVAL:
        /*
         * which was not one of PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
         */
        explain_buffer_einval_vague(sb, "which");
        goto generic;

    case ESRCH:
        /*
         * No process was located using the which and who values specified.
         */
        switch (which)
        {
        case PRIO_PROCESS:
            explain_buffer_esrch(sb, "who", who);
            break;

        case PRIO_PGRP:
            explain_buffer_esrch(sb, "who", (who < 0 ? who : -who));
            break;

        case PRIO_USER:
            explain_string_buffer_puts
            (
                sb,
                /* FIXME:i18n */
                "no process owned by who uid was found"
            );
            break;

        default:
            goto generic;
        }
        break;

    case EACCES:
        /*
         * The caller attempted to lower a process priority, but did
         * not have the required privilege (on Linux: did not have the
         * CAP_SYS_NICE capability).  Since Linux 2.6.12, this error
         * only occurs if the caller attempts to set a process priority
         * outside the range of the RLIMIT_NICE soft resource limit of
         * the target process; see getrlimit(2) for details.
         */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the caller attempted to lower a process priority"
        );
        if (explain_option_dialect_specific())
        {
            /*
             * Since Linux 2.6.12,
             * this error only occurs if the caller attempts to set a
             * process priority outside the range of the RLIMIT_NICE soft
             * resource limit of the target process; see getrlimit(2) for
             * details
             */
            struct rlimit rlim;

#ifdef RLIMIT_NICE
            if (getrlimit(RLIMIT_NICE, &rlim) >= 0)
            {
                int cur = 20 - rlim.rlim_cur;
                explain_string_buffer_printf(sb, " (%d < %d)", prio, cur);
            }
#endif
        }
        explain_buffer_dac_sys_nice(sb);
        break;

    case EPERM:
        /*
         * A process was located, but its effective user ID did not
         * match either the effective or the real user ID of the
         * caller, and was not privileged (on Linux: did not have the
         * CAP_SYS_NICE capability).
         */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "a suitable process was located, but its effective user ID did not "
            "match the effective user ID of this process"
        );

        /* suplementary */
        switch (which)
        {
        case PRIO_PROCESS:
        case PRIO_PGRP:
            /* in this context "who" is a pid_t */
            if (explain_option_dialect_specific())
            {
                int the_other_guy = explain_uid_from_pid(who);
                if (the_other_guy != (pid_t)-1)
                {
                    explain_string_buffer_puts(sb, " (");
                    explain_buffer_uid(sb, the_other_guy);
                    explain_string_buffer_puts(sb, " != ");
                    explain_buffer_uid(sb, geteuid());
                    explain_string_buffer_putc(sb, ')');
                }
            }
            break;

        case PRIO_USER:
            /* in this context "who" is a uid_t */
            if (explain_option_dialect_specific())
            {
                explain_string_buffer_puts(sb, " (");
                explain_buffer_uid(sb, who);
                explain_string_buffer_puts(sb, " != ");
                explain_buffer_uid(sb, geteuid());
                explain_string_buffer_putc(sb, ')');
            }
            break;

        default:
            /* Huh? */
            assert(!"this should have thrown an EINVAL error");
            break;
        }
        explain_buffer_dac_sys_nice(sb);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setpriority(explain_string_buffer_t *sb, int errnum, int
    which, int who, int prio)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setpriority_system_call(&exp.system_call_sb, errnum,
        which, who, prio);
    explain_buffer_errno_setpriority_explanation(&exp.explanation_sb, errnum,
        "setpriority", which, who, prio);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
