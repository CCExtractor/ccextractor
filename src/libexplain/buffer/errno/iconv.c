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

#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/iconv.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_iconv_system_call(explain_string_buffer_t *sb, int errnum,
    iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t
    *outbytesleft)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "iconv(cd = ");
    explain_string_buffer_printf(sb, "%ld", (long)cd);
    explain_string_buffer_puts(sb, ", inbuf = ");
    explain_buffer_pointer(sb, inbuf);
    explain_string_buffer_puts(sb, ", inbytesleft = ");
    explain_buffer_size_t_star(sb, inbytesleft);
    explain_string_buffer_puts(sb, ", outbuf = ");
    explain_buffer_pointer(sb, outbuf);
    explain_string_buffer_puts(sb, ", outbytesleft = ");
    explain_buffer_size_t_star(sb, outbytesleft);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_iconv_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, iconv_t cd, char **inbuf, size_t *inbytesleft,
    char **outbuf, size_t *outbytesleft)
{
    (void)inbuf;
    (void)inbytesleft;
    (void)outbuf;
    (void)outbytesleft;
    switch (errnum)
    {
    case EBADF:
        if (cd == NULL)
        {
            explain_buffer_is_the_null_pointer(sb, "cd");
            break;
        }
        explain_string_buffer_printf
        (
            sb,
            /* FIXME: i18n */
            "the %s argument does not refer to a valid conversion descriptor",
            "cd"
        );
        break;

    case E2BIG:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "there is not sufficient room at *outbuf"
        );
        break;

    case EILSEQ:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "An invalid multi-byte sequence has been encountered in the input"
        );
        break;

    case EINVAL:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "An incomplete multi-byte sequence has been encountered at  "
            "the end of input "
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_iconv(explain_string_buffer_t *sb, int errnum, iconv_t cd,
    char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_iconv_system_call(&exp.system_call_sb, errnum, cd,
        inbuf, inbytesleft, outbuf, outbytesleft);
    explain_buffer_errno_iconv_explanation(&exp.explanation_sb, errnum, "iconv",
        cd, inbuf, inbytesleft, outbuf, outbytesleft);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
