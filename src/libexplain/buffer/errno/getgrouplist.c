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
#include <libexplain/buffer/errno/getgrouplist.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_getgrouplist_system_call(explain_string_buffer_t *sb,
    int errnum, const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getgrouplist(user = ");
    explain_buffer_pathname(sb, user);
    explain_string_buffer_puts(sb, ", group = ");
    explain_buffer_gid(sb, group);
    explain_string_buffer_puts(sb, ", groups = ");
    explain_buffer_pointer(sb, groups);
    explain_string_buffer_puts(sb, ", ngroups = ");
    explain_buffer_int_star(sb, ngroups);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_getgrouplist_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *user, gid_t group,
    gid_t *groups, int *ngroups)
{
    (void)user;
    (void)group;
    (void)groups;
    (void)ngroups;
    switch (errnum)
    {
    case ERANGE:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "there was insufficient space in the groups[] array, the "
            "value returned in *ngroups can be used to resize the "
            "buffer passed to another call to getgrouplist()"

        );
        break;

    default:
        /*
         * There no way to know what went wrong.  Any of the errors
         * reported by setgrent() or getgrent() or endgrent() are possible.
         */
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_getgrouplist(explain_string_buffer_t *sb, int errnum,
    const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getgrouplist_system_call(&exp.system_call_sb, errnum,
        user, group, groups, ngroups);
    explain_buffer_errno_getgrouplist_explanation(&exp.explanation_sb, errnum,
        "getgrouplist", user, group, groups, ngroups);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
