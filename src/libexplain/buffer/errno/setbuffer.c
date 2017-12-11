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

#include <libexplain/buffer/errno/setbuffer.h>
#include <libexplain/buffer/errno/setvbuf.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setbuffer_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp, char *data, size_t size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setbuffer(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", size = ");
    explain_buffer_size_t(sb, size);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setbuffer_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp, char *data, size_t size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setbuffer.html
     */
    explain_buffer_errno_setvbuf_explanation
    (
        sb,
        errnum,
        syscall_name,
        fp,
        data,
        (data ? _IOFBF : _IONBF),
        (data ? size : 0)
    );
}


void
explain_buffer_errno_setbuffer(explain_string_buffer_t *sb, int errnum,
    FILE *fp, char *data, size_t size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setbuffer_system_call(&exp.system_call_sb, errnum, fp,
        data, size);
    explain_buffer_errno_setbuffer_explanation(&exp.explanation_sb, errnum,
        "setbuffer", fp, data, size);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
