/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/putenv.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_putenv_system_call(explain_string_buffer_t *sb, int errnum,
    const char *string)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "putenv(string = ");
    explain_buffer_pathname(sb, string);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_putenv_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *string)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/putenv.html
     */
    (void)string;
    switch (errnum)
    {
    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_putenv(explain_string_buffer_t *sb, int errnum,
    const char *string)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_putenv_system_call(&exp.system_call_sb, errnum,
        string);
    explain_buffer_errno_putenv_explanation(&exp.explanation_sb, errnum,
        "putenv", string);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
