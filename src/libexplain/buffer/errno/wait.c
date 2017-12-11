/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/wait.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/no_outstanding_children.h>
#include <libexplain/buffer/note/sigchld.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_wait_system_call(explain_string_buffer_t *sb,
    int errnum, int *status)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "wait(status = ");
    explain_buffer_pointer(sb, status);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_wait_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int *status)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/wait.html
     */
    (void)status;
    switch (errnum)
    {
    case ECHILD:
        explain_buffer_no_outstanding_children(sb);
        explain_buffer_note_sigchld(sb);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "status");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_wait(explain_string_buffer_t *sb, int errnum,
    int *status)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_wait_system_call
    (
        &exp.system_call_sb,
        errnum,
        status
    );
    explain_buffer_errno_wait_explanation
    (
        &exp.explanation_sb,
        errnum,
        "wait",
        status
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
