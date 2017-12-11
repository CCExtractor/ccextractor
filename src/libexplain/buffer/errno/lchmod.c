/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/buffer/errno/chmod.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/lchmod.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_lchmod_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname, mode_t mode)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "lchmod(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_permission_mode(sb, mode);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_lchmod_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *pathname, mode_t mode)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_modify_inode = 1;
    final_component.follow_symlink = 0;

    explain_buffer_errno_chmod_explanation_fc
    (
        sb,
        errnum,
        syscall_name,
        pathname,
        mode,
        &final_component
    );
}


void
explain_buffer_errno_lchmod(explain_string_buffer_t *sb, int errnum, const char
    *pathname, mode_t mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_lchmod_system_call(&exp.system_call_sb, errnum,
        pathname, mode);
    explain_buffer_errno_lchmod_explanation(&exp.explanation_sb, errnum,
        "lchmod", pathname, mode);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
