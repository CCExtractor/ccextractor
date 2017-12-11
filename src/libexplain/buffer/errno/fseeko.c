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
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/errno/fseeko.h>
#include <libexplain/buffer/errno/lseek.h>
#include <libexplain/buffer/lseek_whence.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fseeko_system_call(explain_string_buffer_t *sb, int errnum,
    FILE *fp, off_t offset, int whence)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fseeko(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_puts(sb, ", offset = ");
    explain_buffer_off_t(sb, offset);
    explain_string_buffer_puts(sb, ", whence = ");
    explain_buffer_lseek_whence(sb, whence);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fseeko_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, FILE *fp, off_t offset, int whence)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fseeko.html
     */
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf_stream(sb, "fp");
        break;

    default:
        explain_buffer_errno_lseek_explanation
        (
            sb,
            errnum,
            syscall_name,
            explain_stream_to_fildes(fp),
            offset,
            whence
        );
        break;
    }
}


void
explain_buffer_errno_fseeko(explain_string_buffer_t *sb, int errnum, FILE *fp,
    off_t offset, int whence)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fseeko_system_call
    (
        &exp.system_call_sb,
        errnum,
        fp,
        offset,
        whence
    );
    explain_buffer_errno_fseeko_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fseeko",
        fp,
        offset,
        whence
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
