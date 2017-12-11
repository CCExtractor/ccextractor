/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/buffer/char_or_eof.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/ungetc.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_ungetc_system_call(explain_string_buffer_t *sb, int errnum,
    int c, FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "ungetc(c = ");
    explain_buffer_char_or_eof(sb, c);
    explain_string_buffer_puts(sb, ", fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_ungetc_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int c, FILE *fp)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/ungetc.html
     */
    switch (errnum)
    {
    case 0:
        explain_buffer_einval_ungetc(sb, syscall_name);
        break;

    case EBADF:
        explain_buffer_ebadf_stream(sb, "fp");
        break;

    case EINVAL:
        if (c == EOF)
        {
            explain_buffer_einval_ungetc(sb, syscall_name);
            break;
        }
        if (explain_stream_to_fildes(fp) == -1)
        {
            explain_buffer_ebadf_stream(sb, "fp");
            break;
        }
        goto generic;

    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_ungetc(explain_string_buffer_t *sb, int errnum, int c,
    FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_ungetc_system_call(&exp.system_call_sb, errnum, c, fp);
    explain_buffer_errno_ungetc_explanation(&exp.explanation_sb, errnum,
        "ungetc", c, fp);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
