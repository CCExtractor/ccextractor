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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getrusage.h>
#include <libexplain/buffer/getrusage_who.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_getrusage_system_call(explain_string_buffer_t *sb,
    int errnum, int who, struct rusage *usage)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getrusage(who = ");
    explain_buffer_getrusage_who(sb, who);
    explain_string_buffer_puts(sb, ", usage = ");
    explain_buffer_pointer(sb, usage);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getrusage_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int who, struct rusage *usage)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getrusage.html
     */
    (void)who;
    (void)usage;
    switch (errnum)
    {
    case EFAULT:
        if (!usage)
        {
            explain_buffer_is_the_null_pointer(sb, "usage");
            break;
        }
        explain_buffer_efault(sb, "usage");
        break;

    case EINVAL:
        explain_buffer_einval_vague(sb, "who");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_getrusage(explain_string_buffer_t *sb, int errnum, int who,
    struct rusage *usage)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getrusage_system_call(&exp.system_call_sb, errnum, who,
        usage);
    explain_buffer_errno_getrusage_explanation(&exp.explanation_sb, errnum,
        "getrusage", who, usage);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
