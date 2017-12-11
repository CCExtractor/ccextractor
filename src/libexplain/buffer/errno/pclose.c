/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/errno/pclose.h>
#include <libexplain/buffer/errno/waitpid.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_pclose_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "pclose(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_pclose_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp)
{
    /* punt everything to waitpid */
    (void)fp;
    explain_buffer_errno_waitpid_explanation(sb, errnum, syscall_name, 0, 0, 0);
}


void
explain_buffer_errno_pclose(explain_string_buffer_t *sb, int errnum,
    FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_pclose_system_call
    (
        &exp.system_call_sb,
        errnum,
        fp
    );
    explain_buffer_errno_pclose_explanation
    (
        &exp.explanation_sb,
        errnum,
        "pclose",
        fp
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
