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

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/calloc.h>
#include <libexplain/buffer/errno/malloc.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_calloc_system_call(explain_string_buffer_t *sb, int errnum,
    size_t nmemb, size_t size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "calloc(nmemb = ");
    explain_buffer_size_t(sb, nmemb);
    explain_string_buffer_puts(sb, ", size = ");
    explain_buffer_size_t(sb, size);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_calloc_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, size_t nmemb, size_t size)
{
    if (errnum == EINVAL)
    {
        /* this is deliberately conservative */
        size_t nmemb_avail = (((size_t)-1) / 3 * 2 / size);
        if (nmemb_avail <= nmemb)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the requested amount of memory exceeds the size "
                "representable by a size_t argument"
            );
            if (explain_option_dialect_specific())
            {
                explain_string_buffer_printf
                (
                    sb,
                    " (%zu > %zu)",
                    nmemb,
                    nmemb_avail
                );
            }
            return;
        }
    }

    explain_buffer_errno_malloc_explanation
    (
        sb,
        errnum,
        syscall_name,
        nmemb * size
    );
}


void
explain_buffer_errno_calloc(explain_string_buffer_t *sb, int errnum, size_t
    nmemb, size_t size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_calloc_system_call(&exp.system_call_sb, errnum, nmemb,
        size);
    explain_buffer_errno_calloc_explanation(&exp.explanation_sb, errnum,
        "calloc", nmemb, size);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
