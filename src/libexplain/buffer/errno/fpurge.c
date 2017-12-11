/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013, 2014 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/errno/fpurge.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fpurge_system_call(explain_string_buffer_t *sb, int errnum,
    FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fpurge(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fpurge_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, FILE *fp)
{
    if (fp == NULL)
    {
        /*
         * Caution: this is one particularly nasty part of the <stdio.h>
         * API.  Actually it means "flush all streams".  Given that we
         * have no access to stdio's list, we can't tell you what went
         * wrong.
         */
        explain_buffer_is_the_null_pointer(sb, "fp");
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fpurge.html
     */
    switch (errnum)
    {
    case EBADF:
        {
            int             fildes;

            fildes = explain_stream_to_fildes(fp);
            explain_buffer_ebadf(sb, fildes, "fp");
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_fpurge(explain_string_buffer_t *sb, int errnum, FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fpurge_system_call(&exp.system_call_sb, errnum, fp);
    explain_buffer_errno_fpurge_explanation(&exp.explanation_sb, errnum,
        "fpurge", fp);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
