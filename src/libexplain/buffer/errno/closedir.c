/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/buffer/dir_to_pathname.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/close.h>
#include <libexplain/buffer/errno/closedir.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/dir_to_fildes.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_closedir_system_call(explain_string_buffer_t *sb,
    int errnum, DIR *dir)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "closedir(dir = ");
    explain_buffer_pointer(sb, dir);
    /* libc has probably nuked it, but we live in hope */
    explain_buffer_dir_to_pathname(sb, dir);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_closedir_explanation(explain_string_buffer_t *sb,
    int errnum, DIR *dir)
{
    int             fildes;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/closedir.html
     */
    if (dir == NULL)
    {
        explain_buffer_is_the_null_pointer(sb, "dir");
        return;
    }
    if (errnum == EFAULT)
    {
        explain_buffer_efault(sb, "dir");
        return;
    }
    fildes = explain_dir_to_fildes(dir);
    if (fildes < 0)
    {
        explain_buffer_ebadf_dir(sb, "dir");
        return;
    }
    explain_buffer_errno_close_explanation(sb, errno, "closedir", fildes);
}


void
explain_buffer_errno_closedir(explain_string_buffer_t *sb, int errnum,
    DIR *dir)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_closedir_system_call
    (
        &exp.system_call_sb,
        errnum,
        dir
    );
    explain_buffer_errno_closedir_explanation
    (
        &exp.explanation_sb,
        errnum,
        dir
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
