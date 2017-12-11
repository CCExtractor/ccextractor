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
#include <libexplain/buffer/errno/strcoll.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_strcoll_system_call(explain_string_buffer_t *sb,
    int errnum, const char *s1, const char *s2)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "strcoll(s1 = ");
    explain_buffer_pathname(sb, s1);
    explain_string_buffer_puts(sb, ", s2 = ");
    explain_buffer_pathname(sb, s2);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_strcoll_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *s1, const char *s2)
{
    (void)s1;
    (void)s2;

    /*
     * This systm call uses malloc() so ENOMEM is possible.  However it
     * also calls the locale functionality which opens and reads files,
     * so there is almost no limit to he errors it could report.
     */
    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_strcoll(explain_string_buffer_t *sb, int errnum,
    const char *s1, const char *s2)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_strcoll_system_call(&exp.system_call_sb, errnum, s1,
        s2);
    explain_buffer_errno_strcoll_explanation(&exp.explanation_sb, errnum,
        "strcoll", s1, s2);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
