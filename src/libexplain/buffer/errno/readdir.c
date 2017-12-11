/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/readdir.h>
#include <libexplain/buffer/dir_to_pathname.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/dir_to_fildes.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_readdir_system_call(explain_string_buffer_t *sb,
    int errnum, DIR *dir)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "readdir(dir = ");
    explain_buffer_pointer(sb, dir);
    explain_buffer_dir_to_pathname(sb, dir);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_readdir_explanation(explain_string_buffer_t *sb,
    int errnum, DIR *dir)
{
    int             fildes;

    /*
     * Most of these errors are from getdents(2).
     */
    if (dir == NULL)
    {
        explain_buffer_is_the_null_pointer(sb, "dir");
        return;
    }
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf_dir(sb, "dir");
        break;

    case EFAULT:
        /*
         * DIR is an opaque type, so we don't really know how big
         * is actually is.  So guess.
         */
        if (explain_is_efault_pointer(dir, sizeof(int)))
            explain_buffer_efault(sb, "dir");
        else
            explain_buffer_efault(sb, "internal buffer");
        break;

    case EINVAL:
        explain_string_buffer_puts
        (
            sb,
            "internal buffer is too small"
        );
        break;

    case EIO:
        fildes = explain_dir_to_fildes(dir);
        explain_buffer_eio_fildes(sb, fildes);
        break;

    case ENOTDIR:
        fildes = explain_dir_to_fildes(dir);
        explain_buffer_enotdir_fd(sb, fildes, "dir");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "readdir");
        break;
    }
}


void
explain_buffer_errno_readdir(explain_string_buffer_t *sb, int errnum,
    DIR *dir)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_readdir_system_call
    (
        &exp.system_call_sb,
        errnum,
        dir
    );
    explain_buffer_errno_readdir_explanation
    (
        &exp.explanation_sb,
        errnum,
        dir
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
