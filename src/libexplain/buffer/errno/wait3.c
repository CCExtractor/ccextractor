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
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/errno/wait3.h>
#include <libexplain/buffer/errno/wait4.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/waitpid_options.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_wait3_system_call(explain_string_buffer_t *sb,
    int errnum, int *status, int options, struct rusage *rusage)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "wait3(status = ");
    explain_buffer_pointer(sb, status);
    explain_string_buffer_puts(sb, ", options = ");
    explain_buffer_waitpid_options(sb, options);
    explain_string_buffer_puts(sb, ", rusage = ");
    explain_buffer_pointer(sb, rusage);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_wait3_explanation(explain_string_buffer_t *sb,
    int errnum, int *status, int options, struct rusage *rusage)
{
    explain_buffer_errno_wait4_explanation
    (
        sb,
        errnum,
        "wait3",
        -1,
        status,
        options,
        rusage
    );
}


void
explain_buffer_errno_wait3(explain_string_buffer_t *sb, int errnum,
    int *status, int options, struct rusage *rusage)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_wait3_system_call
    (
        &exp.system_call_sb,
        errnum,
        status,
        options,
        rusage
    );
    explain_buffer_errno_wait3_explanation
    (
        &exp.explanation_sb,
        errnum,
        status,
        options,
        rusage
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
