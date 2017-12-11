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

#include <libexplain/buffer/errno/vasprintf.h>
#include <libexplain/buffer/errno/vsnprintf.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/va_list.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_vasprintf_system_call(explain_string_buffer_t *sb,
    int errnum, char **data, const char *format, va_list ap)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "vasprintf(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_pathname(sb, format);
    explain_string_buffer_puts(sb, ", ap = ");
    explain_buffer_va_list(sb, ap);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_vasprintf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, char **data, const char *format,
    va_list ap)
{
    char dummy[20];
    if (!data)
    {
        explain_buffer_is_the_null_pointer(sb, "data");
        return;
    }
    explain_buffer_errno_vsnprintf_explanation
    (
        sb,
        errnum,
        syscall_name,
        dummy,
        sizeof(dummy),
        format,
        ap
    );
}


void
explain_buffer_errno_vasprintf(explain_string_buffer_t *sb, int errnum,
    char **data, const char *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_vasprintf_system_call(&exp.system_call_sb, errnum,
        data, format, ap);
    explain_buffer_errno_vasprintf_explanation(&exp.explanation_sb, errnum,
        "vasprintf", data, format, ap);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
