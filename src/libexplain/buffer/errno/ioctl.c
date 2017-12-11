/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/errno/ioctl.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/iocontrol.h>
#include <libexplain/explanation.h>


static int
last_char(const char *s)
{
    if (!s || !*s)
        return 0;
    while (s[1])
        ++s;
    return (unsigned char)*s;
}


static void
system_call(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "ioctl(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", request = ");
    explain_iocontrol_print_name(p, sb, errnum, fildes, request, data);
    explain_string_buffer_puts(sb, ", ");
    if (data && p->data_type)
    {
        const char      *s;
        const char      *a;

        s = p->data_type;
        a = strchr(s, '[');
        if (a)
        {
            const char      *e;

            e = a;
            while (e > s && isspace((unsigned char)e[-1]))
                --e;
            explain_string_buffer_write(sb, s, e - s);
            if (isalpha((unsigned char)e[-1]))
                explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts(sb, "data");
            explain_string_buffer_puts(sb, a);
        }
        else
        {
            explain_string_buffer_puts(sb, p->data_type);
            if (isalpha(last_char(p->data_type)))
                explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts(sb, "data");
        }
    }
    else
        explain_string_buffer_puts(sb, "data");
    explain_string_buffer_puts(sb, " = ");
    explain_iocontrol_print_data(p, sb, errnum, fildes, request, data);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_ioctl(explain_string_buffer_t *sb, int errnum,
    int fildes, int request, void *data)
{
    const explain_iocontrol_t *p;
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    p = explain_iocontrol_find_by_number(fildes, request, data);
    system_call(p, &exp.system_call_sb, errnum, fildes, request, data);
    explain_iocontrol_print_explanation
    (
        p,
        &exp.explanation_sb,
        errnum,
        fildes,
        request,
        data
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
