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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h>

#include <libexplain/buffer/errno/mkstemp.h>
#include <libexplain/buffer/errno/tmpfile.h>
#include <libexplain/explanation.h>
#include <libexplain/path_search.h>


static void
explain_buffer_errno_tmpfile_system_call(explain_string_buffer_t *sb,
    int errnum)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "tmpfile()");
}


void
explain_buffer_errno_tmpfile_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name)
{
    /*
     * We use the same array size that [e]glibc uses.
     */
    char            pathname[FILENAME_MAX];

    /*
     * Do what [e]glibc would do (probably did).
     */
    const char *dir = NULL;
    int try_tmpdir = 0;
    if
    (
        errnum != EFAULT
    &&
        explain_path_search(pathname, sizeof(pathname), dir, "tmpf", try_tmpdir)
    &&
        errno == errnum
    &&
        explain_path_search_explanation(sb, errnum, dir, try_tmpdir) >= 0
    )
    {
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/tmpfile.html
     */
    explain_buffer_errno_mkstemp_explanation
    (
        sb,
        errnum,
        syscall_name,
        pathname
    );
}


void
explain_buffer_errno_tmpfile(explain_string_buffer_t *sb, int errnum)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_tmpfile_system_call(&exp.system_call_sb, errnum);
    explain_buffer_errno_tmpfile_explanation
    (
        &exp.explanation_sb,
        errnum,
        "tmpfile"
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
