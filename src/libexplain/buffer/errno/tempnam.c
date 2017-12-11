/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/dangerous.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/lstat.h>
#include <libexplain/buffer/errno/tempnam.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/path_search.h>


static void
explain_buffer_errno_tempnam_system_call(explain_string_buffer_t *sb,
    int errnum, const char *dir, const char *prefix)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "tempnam(dir = ");
    explain_buffer_pathname(sb, dir);
    explain_string_buffer_puts(sb, ", prefix = ");
    explain_buffer_pathname(sb, prefix);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_tempnam_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *dir, const char *prefix)
{
    /*
     * We use the same array size that [e]glibc uses.
     */
    char            pathname[FILENAME_MAX];

    /*
     * Do what [e]glibc would do (probably did).
     */
    if
    (
        errnum != EFAULT
    &&
        explain_path_search(pathname, sizeof(pathname), dir, prefix, 1) < 0
    &&
        errno == errnum
    &&
        explain_path_search_explanation(sb, errno, dir, 1) >= 0
    )
    {
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/tempnam.html
     */
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EINVAL:
        /*
         * this is particularly weird, but it can happen if the "dir" argument
         * or "prefix" argument is too large.
         */
        explain_buffer_einval_mkstemp(sb, pathname, "pathname");
        break;

    case ENOMEM:
        /*
         * This would be the result of the strdup used to copy the
         * generated pathname.
         */
        explain_buffer_enomem_user(sb, 0);
        break;

    case EEXIST:
        explain_buffer_eexist_tempname_dirname(sb, pathname);
        break;

    default:
        explain_buffer_errno_lstat_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            0
        );
        break;
    }
    explain_buffer_dangerous_system_call(sb, syscall_name);
}


void
explain_buffer_errno_tempnam(explain_string_buffer_t *sb, int errnum,
    const char *dir, const char *prefix)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_tempnam_system_call(&exp.system_call_sb, errnum, dir,
        prefix);
    explain_buffer_errno_tempnam_explanation(&exp.explanation_sb, errnum,
        "tempnam", dir, prefix);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
