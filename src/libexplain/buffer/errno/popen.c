/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/popen.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/string_flags.h>


static void
explain_buffer_errno_popen_system_call(explain_string_buffer_t *sb,
    int errnum, const char *command, const char *flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "popen(command = ");
    explain_buffer_pathname(sb, command);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_pathname(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_popen_explanation(explain_string_buffer_t *sb,
    int errnum, const char *command, const char *flags)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/popen.html
     */
    (void)command;
    switch (errnum)
    {
    case EINVAL:
        {
            explain_string_flags_t sf;

            explain_string_flags_init(&sf, flags);
            explain_string_flags_einval(&sf, sb, "flags");
        }
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENFILE:
        explain_buffer_emfile(sb);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "popen");
        break;
    }
}


void
explain_buffer_errno_popen(explain_string_buffer_t *sb, int errnum,
    const char *command, const char *flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_popen_system_call
    (
        &exp.system_call_sb,
        errnum,
        command,
        flags
    );
    explain_buffer_errno_popen_explanation
    (
        &exp.explanation_sb,
        errnum,
        command,
        flags
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
