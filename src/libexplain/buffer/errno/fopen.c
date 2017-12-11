/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/fopen.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/string_buffer.h>
#include <libexplain/string_flags.h>


static void
explain_buffer_errno_fopen_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, const char *flags_string)
{
    /*
     * Note: EFAULT has to be the pathname, because if flags was broken,
     * it would have raised a SEGFAULT signal from user space.
     */

    explain_string_buffer_printf(sb, "fopen(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_string_buffer_puts_quoted(sb, flags_string);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fopen_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    const char *flags)
{
    explain_string_flags_t sf;
    int             permission_mode;

    explain_string_flags_init(&sf, flags);
    permission_mode = 0666;
    switch (errnum)
    {
    case EINVAL:
        explain_string_flags_einval(&sf, sb, "flags");
        break;

    case ENOMEM:
        {
            int             fd;

            /*
             * Try to figure out if it was a kernel ENOMEM or a user-space
             * (sbrk) ENOMEM.  This is doomed to be inaccurate.
             */
            errno = 0;
            fd = open(pathname, O_RDONLY);
            if (fd < 0)
            {
                if (errno == ENOMEM)
                {
                    explain_buffer_enomem_kernel(sb);
                    break;
                }
                explain_buffer_enomem_kernel_or_user(sb);
                break;
            }
            close(fd);
            explain_buffer_enomem_user(sb, 0);
        }
        break;

    default:
        /*
         * Punt everything else to open()
         */
        explain_buffer_errno_open_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            sf.flags,
            permission_mode
        );
        break;
    }
}


void
explain_buffer_errno_fopen(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const char *flags_string)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fopen_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        flags_string
    );
    explain_buffer_errno_fopen_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fopen",
        pathname,
        flags_string
    );
    explain_explanation_assemble(&exp, sb);
}


void
explain_string_flags_einval(const explain_string_flags_t *sf,
    explain_string_buffer_t *sb, const char *caption)
{
    size_t          n;

    explain_string_buffer_printf
    (
        sb,
        "the %s argument is not valid",
        caption
    );
    if (!sf->rwa_seen)
    {
        explain_string_buffer_puts
        (
            sb,
            ", you must specify 'r', 'w' or 'a' at the start of the string"
        );
    }
    n = strlen(sf->invalid);
    if (n > 0)
    {
        explain_string_buffer_printf
        (
            sb,
            ", flag character%s ",
            (n == 1 ? "" : "s")
        );
        explain_string_buffer_puts_quoted(sb, sf->invalid);
        explain_string_buffer_puts(sb, " unknown");
    }
}


/* vim: set ts=8 sw=4 et : */
