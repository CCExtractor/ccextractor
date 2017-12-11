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

#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/sleep.h>
#include <libexplain/buffer/int.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_sleep_system_call(explain_string_buffer_t *sb, int errnum,
    unsigned int seconds)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "sleep(seconds = ");
    explain_buffer_uint(sb, seconds);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_sleep_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, unsigned int seconds)
{
    (void)seconds;
    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_sleep(explain_string_buffer_t *sb, int errnum, unsigned int
    seconds)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_sleep_system_call(&exp.system_call_sb, errnum,
        seconds);
    explain_buffer_errno_sleep_explanation(&exp.explanation_sb, errnum, "sleep",
        seconds);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
