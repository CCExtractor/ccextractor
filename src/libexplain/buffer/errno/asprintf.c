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
#include <libexplain/ac/stdarg.h>

#include <libexplain/buffer/errno/asprintf.h>
#include <libexplain/buffer/errno/vsnprintf.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_asprintf_system_call(explain_string_buffer_t *sb,
    int errnum, char **data, const char *format, va_list ap)
{
    (void)errnum;
    (void)ap;
    explain_string_buffer_puts(sb, "asprintf(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_pathname(sb, format);

    /*
     * In theory we could print all of the arguments, because we have the
     * format string to tell us their types.  But that's a lot of work,
     * I'll do if someone-asks nicely.
     */
    explain_string_buffer_puts(sb, ", ...");
}


void
explain_buffer_errno_asprintf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, char **data, const char *format,
    va_list ap)
{
    char            dummy[20];

    if (!data)
    {
        explain_buffer_is_the_null_pointer(sb, "data");
        return;
        }

/*/
    case ERANGE: stupid wiht or precision
    case ENOMEM: malloc barfed
    case EINVAL: data or format was NULL
    case EBADF: never, but happens when pint to a file, in the common code
/*/

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
explain_buffer_errno_asprintf(explain_string_buffer_t *sb, int errnum,
    char **data, const char *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_asprintf_system_call
    (
        &exp.system_call_sb,
        errnum,
        data,
        format,
        ap
    );
    explain_buffer_errno_asprintf_explanation
    (
        &exp.explanation_sb,
        errnum,
        "asprintf",
        data,
        format,
        ap
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
