/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/errno/kill.h>
#include <libexplain/buffer/errno/raise.h>
#include <libexplain/buffer/signal.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_raise_system_call(explain_string_buffer_t *sb, int errnum,
    int sig)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "raise(sig = ");
    explain_buffer_signal(sb, sig);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_raise_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int sig)
{
    /*
     * FIXME: In a multithreaded program it is equivalent to
     *
     *     pthread_kill(pthread_self(), sig);
     */
    explain_buffer_errno_kill_explanation
    (
        sb,
        errnum,
        syscall_name,
        getpid(),
        sig
    );
}


void
explain_buffer_errno_raise(explain_string_buffer_t *sb, int errnum, int sig)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_raise_system_call(&exp.system_call_sb, errnum, sig);
    explain_buffer_errno_raise_explanation(&exp.explanation_sb, errnum, "raise",
        sig);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
