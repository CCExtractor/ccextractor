/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/fdopen.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/explanation.h>
#include <libexplain/string_flags.h>


static void
explain_buffer_errno_fdopen_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, const char *flags)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "fdopen(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_string_buffer_puts_quoted(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_fdopen_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, const char *flags)
{
    explain_string_flags_t sf;
    int             xflags;

    explain_string_flags_init(&sf, flags);
    sf.flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fdopen.html
     */
    switch (errnum)
    {
    case EINVAL:
        explain_string_flags_einval(&sf, sb, "flags");
        xflags = fcntl(fildes, F_GETFL);
        if (xflags >= 0 && xflags != sf.flags)
        {
            explain_string_buffer_puts(sb, ", the file descriptor flags (");
            explain_buffer_open_flags(sb, xflags);
            explain_string_buffer_puts
            (
                sb,
                ") do not match the flags specified ("
            );
            explain_buffer_open_flags(sb, sf.flags);
            explain_string_buffer_putc(sb, ')');
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "fdopen");
        break;
    }
}


void
explain_buffer_errno_fdopen(explain_string_buffer_t *sb, int errnum,
    int fildes, const char *flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fdopen_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        flags
    );
    explain_buffer_errno_fdopen_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        flags
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
