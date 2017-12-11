/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013, 2014 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/errno/fgetc.h>
#include <libexplain/buffer/errno/read.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/libio.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fgetc_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fgetc(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fgetc_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp)
{
    int             fildes;

    if (fp == NULL)
    {
        explain_buffer_is_the_null_pointer(sb, "fp");
        return;
    }

    if (errnum == EBADF)
    {
        /*
         * The underlying fildes could be open read/write but the FILE
         * may not be open for writing.
         */
        if (explain_libio_no_reads(fp))
        {
            explain_buffer_ebadf_not_open_for_reading(sb, "fp", -1);
            explain_buffer_software_error(sb);
            return;
        }
        fildes = explain_stream_to_fildes(fp);
        explain_buffer_ebadf(sb, fildes, "fp");
        return;
    }

    fildes = explain_stream_to_fildes(fp);
    explain_buffer_errno_read_explanation
    (
        sb,
        errnum,
        syscall_name,
        fildes,
        NULL,
        0
    );
}


void
explain_buffer_errno_fgetc(explain_string_buffer_t *sb, int errnum,
    FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fgetc_system_call(&exp.system_call_sb, errnum, fp);
    explain_buffer_errno_fgetc_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fgetc",
        fp
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
