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

#include <libexplain/buffer/errno/fork.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/system.h>
#include <libexplain/buffer/errno/wait.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_system_system_call(explain_string_buffer_t *sb,
    int errnum, const char *command)
{
    explain_string_buffer_puts(sb, "system(command = ");
    if (!command)
        explain_string_buffer_puts(sb, "NULL");
    else if (errnum == EFAULT)
        explain_buffer_pointer(sb, command);
    else
        explain_string_buffer_puts_quoted(sb, command);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_system_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *command)
{
    int             junk;

    /*
     * Here is the relevant page of The Single Unix Standard:
     * http://www.opengroup.org/onlinepubs/009695399/functions/system.html
     */
    (void)command;
    switch (errnum)
    {
    case EAGAIN:
    case ENOMEM:
        explain_buffer_errno_fork_explanation(sb, errnum, syscall_name);
        break;

    case ECHILD:
    case EINTR:
    case EINVAL:
        explain_buffer_errno_wait_explanation(sb, errnum, syscall_name, &junk);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_system(explain_string_buffer_t *sb, int errnum,
    const char *command)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_system_system_call
    (
        &exp.system_call_sb,
        errnum,
        command
    );
    explain_buffer_errno_system_explanation
    (
        &exp.explanation_sb,
        errnum,
        "system",
        command
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
