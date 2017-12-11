/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/opendir.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_opendir_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname)
{
    explain_string_buffer_printf(sb, "opendir(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_opendir_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname)
{
    switch (errnum)
    {
    case 0:
        break;

    case ENOMEM:
        {
            int             fd;

            errno = 0;
            fd = open(pathname, O_RDONLY + O_DIRECTORY);
            if (fd < 0)
            {
                if (errno == ENOMEM)
                {
                    explain_buffer_enomem_kernel(sb);
                    return;
                }
                explain_buffer_enomem_kernel_or_user(sb);
                return;
            }
            close(fd);
            explain_buffer_enomem_user(sb, 0);
        }
        break;

    default:
        explain_buffer_errno_open_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            O_RDONLY + O_DIRECTORY,
            0
        );
        break;
    }
}


void
explain_buffer_errno_opendir(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_opendir_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname
    );
    explain_buffer_errno_opendir_explanation
    (
        &exp.explanation_sb,
        errnum,
        "opendir",
        pathname
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
