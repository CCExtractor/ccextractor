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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/vsnprintf.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/va_list.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/printf_format.h>


static void
explain_buffer_errno_vsnprintf_system_call(explain_string_buffer_t *sb,
    int errnum, char *data, size_t data_size, const char *format, va_list ap)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "vsnprintf(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_pathname(sb, format);
    explain_string_buffer_puts(sb, ", ap = ");
    explain_buffer_va_list(sb, ap);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_vsnprintf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, char *data, size_t data_size,
    const char *format, va_list ap)
{
    int             cur_idx;
    size_t          j;
    explain_printf_format_list_t specs;
    size_t          errpos;

    (void)errnum;
    (void)syscall_name;
    (void)ap;
    (void)data_size;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/vsnprintf.html
     */
    if (!data)
    {
        explain_buffer_is_the_null_pointer(sb, "data");
        return;
    }
    if (!format)
    {
        explain_buffer_is_the_null_pointer(sb, "format");
        return;
    }
    if (explain_is_efault_string(format))
    {
        explain_buffer_efault(sb, "format");
        return;
    }

    /*
     * Check the format string.
     * All the fugly stuff is hidden in explain_printf_format().
     */
    explain_printf_format_list_constructor(&specs);
    errpos = explain_printf_format(format, &specs);
    if (errpos > 0)
    {
        explain_buffer_einval_format_string
        (
            sb,
            "format",
            format,
            errpos
        );
        explain_printf_format_list_destructor(&specs);
        return;
    }
    explain_printf_format_list_sort(&specs);
    /* duplicates are OK, holes are not */
    cur_idx = 0;
    for (j = 0; j < specs.length; ++j)
    {
        int             idx;

        idx = specs.list[j].index;
        if (idx > cur_idx)
        {
            /* we found a hole */
            explain_buffer_einval_format_string_hole
            (
                sb,
                "format",
                cur_idx + 1
            );
            explain_printf_format_list_destructor(&specs);
            return;
        }
        if (idx == cur_idx)
            ++cur_idx;
    }
    explain_printf_format_list_destructor(&specs);

    /*
     * No idea.
     */
    explain_buffer_einval_vague(sb, "format");
}


void
explain_buffer_errno_vsnprintf(explain_string_buffer_t *sb, int errnum, char
    *data, size_t data_size, const char *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_vsnprintf_system_call(&exp.system_call_sb, errnum,
        data, data_size, format, ap);
    explain_buffer_errno_vsnprintf_explanation(&exp.explanation_sb, errnum,
        "vsnprintf", data, data_size, format, ap);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
