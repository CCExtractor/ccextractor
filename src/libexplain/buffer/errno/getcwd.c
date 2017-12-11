/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/errno.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getcwd.h>
#include <libexplain/buffer/get_current_directory.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_getcwd_system_call(explain_string_buffer_t *sb,
    int errnum, char *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "getcwd(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_printf(sb, ", data_size = %ld)", (long)data_size);
}


static void
explain_buffer_errno_getcwd_explanation(explain_string_buffer_t *sb,
    int errnum, char *data, size_t data_size)
{
    char            pathname[PATH_MAX * 2 + 1];

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getcwd.html
     */
    switch (errnum)
    {
    case EINVAL:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the data_size argument is zero and data is not the NULL pointer"
        );
        break;

    case ERANGE:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the data_size argument is less than the length of the "
            "working directory name, you need to allocate a bigger "
            "array and try again"
        );
        if (data && data_size && explain_option_dialect_specific())
        {
            explain_string_buffer_t nowhere;

            explain_string_buffer_init(&nowhere, 0, 0);
            if
            (
                !explain_buffer_get_current_directory
                (
                    &nowhere,
                    pathname,
                    sizeof(pathname)
                )
            )
            {
                explain_string_buffer_printf
                (
                    sb,
                    " (%ld < %ld)",
                    (long)data_size,
                    (long)(strlen(pathname) + 1)
                );
            }
        }
        break;

    case EACCES:
        if
        (
            !explain_buffer_get_current_directory
            (
                sb,
                pathname,
                sizeof(pathname)
            )
        )
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "read or search permission was denied for a component "
                "of the pathname"
            );
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case ENOENT:
        if
        (
            !explain_buffer_get_current_directory
            (
                sb,
                pathname,
                sizeof(pathname)
            )
        )
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the current working directory has been unlinked"
            );
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "getcwd");
        break;
    }
}


void
explain_buffer_errno_getcwd(explain_string_buffer_t *sb, int errnum,
    char *data, size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getcwd_system_call
    (
        &exp.system_call_sb,
        errnum,
        data,
        data_size
    );
    explain_buffer_errno_getcwd_explanation
    (
        &exp.explanation_sb,
        errnum,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
