/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/fputs.h>
#include <libexplain/buffer/errno/vfprintf.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/buffer/va_list.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/libio.h>
#include <libexplain/printf_format.h>


static void
explain_buffer_errno_vfprintf_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp, const char *format, va_list ap)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "vfprintf(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_pathname(sb, format);
    explain_string_buffer_puts(sb, ", ap = ");
    explain_buffer_va_list(sb, ap);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_vfprintf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp, const char *format,
    va_list ap)
{
    if (!fp)
    {
        explain_buffer_is_the_null_pointer(sb, "fp");
        return;
    }
    if (explain_is_efault_pointer(fp, sizeof(*fp)))
    {
        explain_buffer_efault(sb, "fp");
        return;
    }

    if (errnum == EBADF)
    {
        /*
         * The underlying fildes could be open read/write but the FILE
         * may not be open for writing.
         */
        if (explain_libio_no_writes(fp))
        {
            explain_buffer_ebadf_not_open_for_writing(sb, "fp", -1);
            explain_buffer_software_error(sb);
            return;
        }
    }

    if (!format)
    {
        explain_buffer_is_the_null_pointer(sb, "format");
        return;
    }
    if (explain_is_efault_string(format))
    {
        explain_buffer_efault(sb, "format");
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/vfprintf.html
     */
    (void)ap;
    switch (errnum)
    {
    case EBADF:
        /* let "fputs" explain it */
        goto generic;

    case EINVAL:
        /* fp could be NULL, but we already checked for that */

        /*
         * Check the format string.
         * All the fugly stuff is hidden in explain_printf_format().
         */
        {
            int             cur_idx;
            size_t          j;
            explain_printf_format_list_t specs;
            size_t          errpos;

            explain_printf_format_list_constructor(&specs);
            errpos = explain_printf_format(format, &specs);
            if (errpos > 0)
            {
                explain_buffer_einval_format_string
                (
                    sb,
                    "format",
                    format,
                    errpos
                );
                explain_printf_format_list_destructor(&specs);
                return;
            }
            explain_printf_format_list_sort(&specs);
            /* duplicates are OK, holes are not */
            cur_idx = 0;
            for (j = 0; j < specs.length; ++j)
            {
                int             idx;

                idx = specs.list[j].index;
                if (idx > cur_idx)
                {
                    /* we found a hole */
                    explain_buffer_einval_format_string_hole
                    (
                        sb,
                        "format",
                        cur_idx + 1
                    );
                    explain_printf_format_list_destructor(&specs);
                    return;
                }
                if (idx == cur_idx)
                    ++cur_idx;
            }
            explain_printf_format_list_destructor(&specs);
        }

#if 0
#ifdef HAVE_FWIDE
        /*
         * FIXME: [e]glibc returns EOF (without setting errno) if the stream
         * is wide (this is a narrow char printf).
         *
         * Our recommended usage is to set errno = EINVAL before the call.
         */
        if (fwide(fp, 0) > 0)
        {
            "The stream is for wide characters, and this is a narrow character "
            "operation.  We need to do this check for all narrow character     "
            "functions.                                                        "
            return;
        }
#endif
#endif
        goto generic;

    default:
        generic:
        explain_buffer_errno_fputs_explanation
        (
            sb,
            errnum,
            syscall_name,
            format,
            fp
        );
        break;
    }
}


void
explain_buffer_errno_vfprintf(explain_string_buffer_t *sb, int errnum, FILE *fp,
    const char *format, va_list ap)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_vfprintf_system_call(&exp.system_call_sb, errnum, fp,
        format, ap);
    explain_buffer_errno_vfprintf_explanation(&exp.explanation_sb, errnum,
        "vfprintf", fp, format, ap);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
