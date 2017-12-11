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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/munmap.h>
#include <libexplain/buffer/must_be_multiple_of_page_size.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>
#include <libexplain/getpagesize.h>


static void
explain_buffer_errno_munmap_system_call(explain_string_buffer_t *sb, int errnum,
    void *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "munmap(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_munmap_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, void *data, size_t data_size)
{
    int             page_size;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/munmap.html
     *
     * Note: it is not an error to munmap an address range that is not
     * mapped, an nothing is reported.
     */
    switch (errnum)
    {
    case EINVAL:
        page_size = explain_getpagesize();
        if (page_size > 0)
        {
            int mask = page_size - 1;
            if ((unsigned long)data & mask)
            {
                explain_buffer_must_be_multiple_of_page_size(sb, "data");
                break;
            }
            if ((unsigned long)data_size & mask)
            {
                explain_buffer_must_be_multiple_of_page_size(sb, "data_size");
                break;
            }
        }
        if (data_size == 0)
        {
            explain_buffer_einval_too_small(sb, "data_size", data_size);
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_munmap(explain_string_buffer_t *sb, int errnum, void *data,
    size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_munmap_system_call(&exp.system_call_sb, errnum, data,
        data_size);
    explain_buffer_errno_munmap_explanation(&exp.explanation_sb, errnum,
        "munmap", data, data_size);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
