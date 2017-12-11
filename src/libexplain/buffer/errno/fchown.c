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

#include <libexplain/buffer/errno/chown.h>
#include <libexplain/buffer/errno/fchown.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fchown_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, int owner, int group)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "fchown(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", owner = ");
    explain_buffer_uid(sb, owner);
    explain_string_buffer_puts(sb, ", group = ");
    explain_buffer_gid(sb, group);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fchown(explain_string_buffer_t *sb, int errnum,
    int fildes, int owner, int group)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fchown_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        owner,
        group
    );
    explain_buffer_errno_fchown_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fchown",
        fildes,
        owner,
        group,
        "fildes"
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
