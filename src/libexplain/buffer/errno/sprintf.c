/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/limits.h>

#include <libexplain/buffer/dangerous.h>
#include <libexplain/buffer/errno/snprintf.h>
#include <libexplain/buffer/errno/sprintf.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/printf_format.h>


static void
explain_buffer_errno_sprintf_system_call(explain_string_buffer_t *sb, int
    errnum, char *data, const char *format, va_list ap)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "sprintf(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_pathname(sb, format);
    if (format && !explain_is_efault_string(format))
        explain_printf_format_representation(sb, format, ap);
    explain_string_buffer_putc(sb, ')');

    explain_buffer_dangerous_system_call2(sb->footnotes, "sprintf", "snprintf");
}


void
explain_buffer_errno_sprintf_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, char *data, const char *format,
    va_list ap)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/sprintf.html
     */
    explain_buffer_errno_snprintf_explanation
    (
        sb,
        errnum,
        syscall_name,
        data,
        INT_MAX, /* this is why it's dangerous */
        format,
        ap
    );
}


void
explain_buffer_errno_sprintf(explain_string_buffer_t *sb, int errnum, char
    *data, const char *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_sprintf_system_call
    (
        &exp.system_call_sb,
        errnum,
        data,
        format,
        ap
    );
    explain_buffer_errno_sprintf_explanation
    (
        &exp.explanation_sb,
        errnum,
        "sprintf",
        data,
        format,
        ap
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
