/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/dir_to_pathname.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/telldir.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/dir_to_fildes.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_telldir_system_call(explain_string_buffer_t *sb, int
    errnum, DIR *dir)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "telldir(dir = ");
    explain_buffer_pointer(sb, dir);
    explain_buffer_dir_to_pathname(sb, dir);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_telldir_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, DIR *dir)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/telldir.html
     */
    if (dir == NULL)
    {
        explain_buffer_is_the_null_pointer(sb, "dir");
        return;
    }
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, explain_dir_to_fildes(dir), "dir");
        break;

    case EFAULT:
        explain_buffer_efault(sb, "dir");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_telldir(explain_string_buffer_t *sb, int errnum, DIR *dir)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_telldir_system_call(&exp.system_call_sb, errnum, dir);
    explain_buffer_errno_telldir_explanation(&exp.explanation_sb, errnum,
        "telldir", dir);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
