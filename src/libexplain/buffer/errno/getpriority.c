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
#include <libexplain/ac/sys/time.h>
#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getpriority.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/prio_which.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/buffer/int.h>
#include <libexplain/option.h>
#include <libexplain/explanation.h>
#include <libexplain/uid_from_pid.h>


static void
explain_buffer_errno_getpriority_system_call(explain_string_buffer_t *sb, int
    errnum, int which, int who)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getpriority(which = ");
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
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getpriority_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int which, int who)
{
    switch (errnum)
    {
    case EINVAL:
        /*
         * which was not one of PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
         */
        switch (which)
        {
        case PRIO_PROCESS:
        case PRIO_PGRP:
        case PRIO_USER:
            goto generic;

        default:
            explain_buffer_einval_vague(sb, "which");
            break;
        }
        break;

    case ESRCH:
        /*
         * No process was located using the which and who values specified.
         */
        switch (which)
        {
        case PRIO_PROCESS:
            explain_buffer_esrch(sb, "who", who);
            return;

        case PRIO_PGRP:
            explain_buffer_esrch(sb, "who", -who);
            return;

        default:
            explain_string_buffer_puts
            (
                sb,
                "no process was located using the which and who values "
                "specified"
            );
            goto generic;
        }

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
            " A process was located, but its effective user ID did not "
            " match the effective user ID of this process"
        );

        /* suplementary */
        if (explain_option_dialect_specific())
        {
            int the_other_guy = explain_uid_from_pid(who);
            if (the_other_guy > 0)
            {
                explain_string_buffer_puts(sb, " (");
                explain_buffer_uid(sb, the_other_guy);
                explain_string_buffer_puts(sb, " != ");
                explain_buffer_uid(sb, geteuid());
                explain_string_buffer_putc(sb, ')');
            }
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
explain_buffer_errno_getpriority(explain_string_buffer_t *sb, int errnum, int
    which, int who)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getpriority_system_call(&exp.system_call_sb, errnum,
        which, who);
    explain_buffer_errno_getpriority_explanation(&exp.explanation_sb, errnum,
        "getpriority", which, who);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
