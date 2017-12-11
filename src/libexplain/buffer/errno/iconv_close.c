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

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/iconv_close.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_iconv_close_system_call(explain_string_buffer_t *sb,
    int errnum, iconv_t cd)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "iconv_close(cd = ");
    explain_buffer_pointer(sb, cd);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_iconv_close_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, iconv_t cd)
{
    (void)cd;
    switch (errnum)
    {
    case EBADF:
        /*
         * You have to read the eglibc source to know this.
         * It isn't in the man page.
         */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the cd argument does not refer to a valid conversion descriptor"
            /* see also iconv() it has the same error */
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_iconv_close(explain_string_buffer_t *sb, int errnum,
    iconv_t cd)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_iconv_close_system_call(&exp.system_call_sb, errnum,
        cd);
    explain_buffer_errno_iconv_close_explanation(&exp.explanation_sb, errnum,
        "iconv_close", cd);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
