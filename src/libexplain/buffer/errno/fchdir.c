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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/fchdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/explanation.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_fchdir_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "fchdir(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_fchdir_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes)
{
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ENOTDIR:
        explain_buffer_enotdir_fd(sb, fildes, "fildes");
        break;

    case EACCES:
        explain_string_buffer_puts
        (
            sb,
            "search permission was denied on the directory"
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "fchdir");
        break;
    }
}


void
explain_buffer_errno_fchdir(explain_string_buffer_t *sb, int errnum,
    int fildes)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fchdir_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes
    );
    explain_buffer_errno_fchdir_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
