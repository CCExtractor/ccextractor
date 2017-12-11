/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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
#include <libexplain/ac/stdlib.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/realloc.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_realloc_system_call(explain_string_buffer_t *sb,
    int errnum, void *ptr, size_t size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "realloc(ptr = ");
    explain_buffer_pointer(sb, ptr);
    explain_string_buffer_printf(sb, ", size = %lu)", (unsigned long)size);
}


static void
explain_buffer_errno_realloc_explanation(explain_string_buffer_t *sb,
    int errnum, void *ptr, size_t size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/realloc.html
     */
    (void)ptr;
    (void)size;
    switch (errnum)
    {
    case ENOMEM:
        explain_buffer_enomem_user(sb, size);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "realloc");
        break;
    }
}


void
explain_buffer_errno_realloc(explain_string_buffer_t *sb, int errnum,
    void *ptr, size_t size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_realloc_system_call
    (
        &exp.system_call_sb,
        errnum,
        ptr,
        size
    );
    explain_buffer_errno_realloc_explanation
    (
        &exp.explanation_sb,
        errnum,
        ptr,
        size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
