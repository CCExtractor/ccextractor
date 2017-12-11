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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getpgrp.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_getpgrp_system_call(explain_string_buffer_t *sb,
    int errnum, pid_t pid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getpgrp(");
#if GETPGRP_NARGS > 0
    explain_string_buffer_puts(sb, "pid = ");
    explain_buffer_pid_t(sb, pid);
#else
    if (pid != 0)
    {
        explain_string_buffer_puts(sb, "pid = ");
        explain_buffer_pid_t(sb, pid);
    }
#endif
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getpgrp_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, pid_t pid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getpgrp.html
     */
    switch (errnum)
    {
    case EINVAL:
        too_small:
        explain_buffer_einval_too_small(sb, "pid", pid);
        break;

    case ESRCH:
        if (pid < 0)
            goto too_small;
        explain_buffer_esrch(sb, "pid", pid);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_getpgrp(explain_string_buffer_t *sb, int errnum, pid_t pid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getpgrp_system_call(&exp.system_call_sb, errnum, pid);
    explain_buffer_errno_getpgrp_explanation(&exp.explanation_sb, errnum,
        "getpgrp", pid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
