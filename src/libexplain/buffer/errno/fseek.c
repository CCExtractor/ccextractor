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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/fseek.h>
#include <libexplain/buffer/errno/lseek.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/lseek_whence.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fseek_system_call(explain_string_buffer_t *sb, int errnum,
    FILE *fp, long offset, int whence)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fseek(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_puts(sb, ", offset = ");
    explain_buffer_long(sb, offset);
    explain_string_buffer_puts(sb, ", whence = ");
    explain_buffer_lseek_whence(sb, whence);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fseek_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, FILE *fp, long offset, int whence)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fseek.html
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
explain_buffer_errno_fseek(explain_string_buffer_t *sb, int errnum, FILE *fp,
    long offset, int whence)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fseek_system_call(&exp.system_call_sb, errnum, fp,
        offset, whence);
    explain_buffer_errno_fseek_explanation(&exp.explanation_sb, errnum, "fseek",
        fp, offset, whence);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
