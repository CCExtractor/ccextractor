/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/errno/creat.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_creat_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int mode)
{
    explain_string_buffer_printf(sb, "creat(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_permission_mode(sb, mode);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_creat_explanation(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int mode)
{
    int             flags;

    flags = O_WRONLY | O_CREAT | O_TRUNC;
    explain_buffer_errno_open_explanation
    (
        sb,
        errnum,
        "creat",
        pathname,
        flags,
        mode
    );
}


void
explain_buffer_errno_creat(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_creat_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        mode
    );
    explain_buffer_errno_creat_explanation
    (
        &exp.explanation_sb,
        errnum,
        pathname,
        mode
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
