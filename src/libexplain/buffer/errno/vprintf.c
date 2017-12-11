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

#include <libexplain/buffer/errno/vfprintf.h>
#include <libexplain/buffer/errno/vprintf.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/va_list.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_vprintf_system_call(explain_string_buffer_t *sb,
    int errnum, const char *format, va_list ap)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "vprintf(format = ");
    explain_buffer_pathname(sb, format);
    explain_string_buffer_puts(sb, ", ap = ");
    explain_buffer_va_list(sb, ap);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_vprintf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *format, va_list ap)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/vprintf.html
     */
    explain_buffer_errno_vfprintf_explanation
    (
        sb,
        errnum,
        syscall_name,
        stdout,
        format,
        ap
    );
}


void
explain_buffer_errno_vprintf(explain_string_buffer_t *sb, int errnum, const char
    *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_vprintf_system_call(&exp.system_call_sb, errnum,
        format, ap);
    explain_buffer_errno_vprintf_explanation(&exp.explanation_sb, errnum,
        "vprintf", format, ap);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
