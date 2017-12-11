/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/errno/fclose.h>
#include <libexplain/buffer/errno/fflush.h>
#include <libexplain/buffer/errno/fopen.h>
#include <libexplain/buffer/errno/freopen.h>
#include <libexplain/buffer/note/underlying_fildes_open.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_freopen_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, const char *flags, FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "freopen(pathname = ");
    explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_string_buffer_puts_quoted(sb, flags);
    explain_string_buffer_puts(sb, ", fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_freopen_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    const char *flags, FILE *fp)
{
    switch (errnum)
    {
    case EFAULT:
    case EFBIG:
    case ENOSPC:
    case EPIPE:
        explain_buffer_errno_fflush_explanation(sb, errnum, syscall_name, fp);
        return;

    case EBADF:
        explain_buffer_ebadf(sb, explain_stream_to_fildes(fp), "fp");
        break;

    case EINTR:
    case EIO:
        explain_buffer_errno_fclose_explanation(sb, errnum, syscall_name, fp);
        return;

    default:
        explain_buffer_errno_fopen_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            flags
        );
        break;
    }

    explain_buffer_note_underlying_fildes_open(sb);
}


void
explain_buffer_errno_freopen(struct explain_string_buffer_t *sb,
    int errnum, const char *pathname, const char *flags, FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_freopen_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        flags,
        fp
    );
    explain_buffer_errno_freopen_explanation
    (
        &exp.explanation_sb,
        errnum,
        "freopen",
        pathname,
        flags,
        fp
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
