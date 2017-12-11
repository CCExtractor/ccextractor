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

#include <libexplain/buffer/errno/malloc.h>
#include <libexplain/buffer/errno/strdup.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_strdup_system_call(explain_string_buffer_t *sb, int errnum,
    const char *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "strdup(data = ");
    explain_buffer_pathname(sb, data);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_strdup_explanation(explain_string_buffer_t *sb, int errnum,
    const char *data)
{
    (void)data;
    explain_buffer_errno_malloc_explanation(sb, errnum, "strdup", 1);
}


void
explain_buffer_errno_strdup(explain_string_buffer_t *sb, int errnum,
    const char *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_strdup_system_call(&exp.system_call_sb, errnum, data);
    explain_buffer_errno_strdup_explanation(&exp.explanation_sb, errnum, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
