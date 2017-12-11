/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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
#include <libexplain/buffer/errno/lstat.h>
#include <libexplain/buffer/errno/tmpnam.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/path_search.h>


static void
explain_buffer_errno_tmpnam_system_call(explain_string_buffer_t *sb, int errnum,
    char *pathname)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "tmpnam(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_tmpnam_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, char *pathname)
{
    char            *tmpbuf;
    const char      *dir;
    const char      *prefix;
    int             try_tmpdir;
    char            tmpbufmem[L_tmpnam];

    tmpbuf = pathname ? pathname : tmpbufmem;
    dir = NULL;
    prefix = 0;
    try_tmpdir = 0;
    if
    (
        errnum != EFAULT
    &&
        explain_path_search(tmpbuf, L_tmpnam, dir, prefix, try_tmpdir) < 0
    &&
        errno == errnum
    &&
        explain_path_search_explanation(sb, errnum, dir, try_tmpdir)
    )
    {
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/tmpnam.html
     */
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EEXIST:
        explain_buffer_eexist_tempname_dirname(sb, tmpbuf);
        break;

    case EINVAL:
        explain_buffer_einval_mkstemp(sb, tmpbuf, "pathname");
        break;

    default:
        explain_buffer_errno_lstat_explanation
        (
            sb,
            errnum,
            syscall_name,
            tmpbuf,
            0
        );
        break;
    }
    explain_buffer_dangerous_system_call(sb, syscall_name);
}


void
explain_buffer_errno_tmpnam(explain_string_buffer_t *sb, int errnum, char
    *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_tmpnam_system_call(&exp.system_call_sb, errnum,
        pathname);
    explain_buffer_errno_tmpnam_explanation(&exp.explanation_sb, errnum,
        "tmpnam", pathname);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
