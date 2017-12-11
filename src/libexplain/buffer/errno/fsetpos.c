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
#include <libexplain/buffer/errno/fsetpos.h>
#include <libexplain/buffer/fpos_t.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fsetpos_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp, fpos_t *pos)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fsetpos(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_puts(sb, ", pos = ");
    explain_buffer_fpos_t(sb, pos, 1);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fsetpos_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp, fpos_t *pos)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fsetpos.html
     */
    (void)fp;
    (void)pos;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf_stream(sb, "fp");
        break;

    case EIO:
    case EINVAL:
        explain_buffer_einval_vague(sb, "pos");
        break;

    default:
        explain_buffer_errno_fseek_explanation
        (
            sb,
            errnum,
            syscall_name,
            fp,
#ifdef HAVE__G_CONFIG_H
            pos->__pos,
#else
            0L,
#endif
            SEEK_SET
        );
        break;
    }
}


void
explain_buffer_errno_fsetpos(explain_string_buffer_t *sb, int errnum, FILE *fp,
    fpos_t *pos)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fsetpos_system_call(&exp.system_call_sb, errnum, fp,
        pos);
    explain_buffer_errno_fsetpos_explanation(&exp.explanation_sb, errnum,
        "fsetpos", fp, pos);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
