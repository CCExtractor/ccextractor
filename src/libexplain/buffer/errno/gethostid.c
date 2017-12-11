/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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
#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/errno/gethostid.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


#ifndef HOSTIDFILE
#define HOSTIDFILE "/etc/hostid"
#endif


static void
explain_buffer_errno_gethostid_system_call(explain_string_buffer_t *sb,
    int errnum)
{
    explain_string_buffer_puts(sb, "gethostid(");
    if (errnum == ENOENT)
    {
        explain_string_buffer_puts(sb, "pathname = ");
        explain_buffer_pathname(sb, "/etc/hostid");
    }
    explain_string_buffer_puts(sb, ")");
}


void
explain_buffer_errno_gethostid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name)
{
    /*
     * Probably actually an
     *
     *     open(HOSTIDFILE, O_RDONLY)
     *
     * error.
     */
    explain_buffer_errno_open_explanation
    (
        sb,
        errnum,
        syscall_name,
        HOSTIDFILE,
        O_RDONLY,
        0
    );
}


void
explain_buffer_errno_gethostid(explain_string_buffer_t *sb, int errnum)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_gethostid_system_call(&exp.system_call_sb, errnum);
    explain_buffer_errno_gethostid_explanation(&exp.explanation_sb, errnum,
        "gethostid");
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
