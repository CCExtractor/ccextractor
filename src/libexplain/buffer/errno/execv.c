/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/execv.h>
#include <libexplain/buffer/errno/execve.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/have_permission.h>
#include <libexplain/name_max.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


static size_t
count(char *const *p)
{
    size_t          result;

    result = 0;
    while (*p)
    {
        ++result;
        ++p;
    }
    return result;
}


static void
explain_buffer_errno_execv_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, char *const *argv)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "execv(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", argv = ");
    if (explain_is_efault_pointer(argv, sizeof(argv[0])))
        explain_buffer_pointer(sb, argv);
    else
    {
        size_t          argc;
        size_t          n;
        size_t          argsize;

        /*
         * produce output similar to execve
         */
        argc = count(argv);
        explain_string_buffer_putc(sb, '[');
        argsize = 0;
        for (n = 0; n < argc && argsize < 1000; ++n)
        {
            const char      *s;

            if (n)
                explain_string_buffer_puts(sb, ", ");
            s = argv[n];
            if (explain_is_efault_path(s))
            {
                explain_buffer_pointer(sb, s);
                argsize += 8;
            }
            else
            {
                explain_string_buffer_puts_quoted(sb, s);
                argsize += strlen(s);
            }
        }
        if (n < argc)
        {
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "... plus another %d command line arguments",
                (int)(count(argv) - n)
            );
        }
        explain_string_buffer_putc(sb, ']');
    }
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_execv_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    char *const *argv)
{
    explain_buffer_errno_execve_explanation
    (
        sb,
        errnum,
        syscall_name,
        pathname,
        argv,
        environ
    );
}


void
explain_buffer_errno_execv(explain_string_buffer_t *sb, int errnum,
    const char *pathname, char *const *argv)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_execv_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        argv
    );
    explain_buffer_errno_execv_explanation
    (
        &exp.explanation_sb,
        errnum,
        "execv",
        pathname,
        argv
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
