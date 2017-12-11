/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/realpath.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_realpath_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, char *resolved_pathname)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "realpath(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", resolved_pathname = ");
    explain_buffer_pointer(sb, resolved_pathname);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_realpath_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    char *resolved_pathname)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.follow_symlink = 1;
    final_component.must_exist = 1;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/realpath.html
     */
    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EINVAL:
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            return;
        }
        if (!resolved_pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "resolved_pathname");
            return;
        }
        explain_buffer_einval_vague(sb, "pathname");
        break;

    case EIO:
        explain_buffer_eio(sb);
        break;

    case ELOOP:
    case EMLINK: /* BSD */
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong
        (
            sb,
            pathname,
            "pathname",
            &final_component
        );
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_realpath(explain_string_buffer_t *sb, int errnum,
    const char *pathname, char *resolved_pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_realpath_system_call(&exp.system_call_sb, errnum,
        pathname, resolved_pathname);
    explain_buffer_errno_realpath_explanation(&exp.explanation_sb, errnum,
        "realpath", pathname, resolved_pathname);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
