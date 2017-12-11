/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/fflush.h>
#include <libexplain/buffer/errno/write.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fflush_system_call(explain_string_buffer_t *sb, int errnum,
    FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fflush(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fflush_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp)
{
    int             fildes;

    /*
     * FIXME:  "If the stream argument is NULL, fflush() flushes all open
     * output streams."
     *
     * -- How do we tell which one had the problem?
     * -- How do we even find them all?
     */

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fflush.html
     */
    switch (errnum)
    {
    case EBADF:
    case EINVAL:
        explain_buffer_ebadf_stream(sb, "fp");
        break;

    default:
        fildes = explain_stream_to_fildes(fp);
        explain_buffer_errno_write_explanation
        (
            sb,
            errnum,
            syscall_name,
            fildes,
            NULL,
            0
        );
        break;
    }
}


void
explain_buffer_errno_fflush(explain_string_buffer_t *sb, int errnum, FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fflush_system_call(&exp.system_call_sb, errnum, fp);
    explain_buffer_errno_fflush_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fflush",
        fp
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
