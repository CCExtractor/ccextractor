/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/buffer/errno/utime.h>
#include <libexplain/buffer/errno/utimes.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_utimes_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const struct timeval *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "utimes(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_timeval_array(sb, data, 2);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_utimes_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *pathname, const struct timeval *data)
{
    explain_buffer_errno_utime_explanation
    (
        sb,
        errnum,
        syscall_name,
        pathname,
        (const struct utimbuf *)data
    );
}


void
explain_buffer_errno_utimes(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const struct timeval *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_utimes_system_call(&exp.system_call_sb, errnum,
        pathname, data);
    explain_buffer_errno_utimes_explanation(&exp.explanation_sb, errnum,
        "utimes", pathname, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
