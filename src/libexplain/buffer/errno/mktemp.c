/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
#include <libexplain/ac/string.h>

#include <libexplain/buffer/dangerous.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/lstat.h>
#include <libexplain/buffer/errno/mktemp.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_mktemp_system_call(explain_string_buffer_t *sb, int errnum,
    char *pathname)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "mktemp(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_mktemp_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, char *pathname)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/mktemp.html
     */
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EINVAL:
        explain_buffer_einval_mkstemp(sb, pathname, "pathname");
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
            NULL
        );
        break;
    }
    explain_buffer_dangerous_system_call(sb, syscall_name);
}


void
explain_buffer_errno_mktemp(explain_string_buffer_t *sb, int errnum,
    char *pathname)
{
    char            ok;
    char            first;
    explain_explanation_t exp;

    /*
     * From mktemp(3) man page...
     *
     *     The mktemp() function always returns pathname.  If a unique name was
     *     created, the last six bytes of pathname will have been modified in
     *     such a way that the resulting name is unique (i.e., does not exist
     *     already) If a unique name could not be created, pathname is made an
     *     empty string.
     *
     * Morons.  Returning NULL would have been far more uesful, not to mention
     * far more consistent.
     */
    ok = !explain_is_efault_string(pathname);
    first = '\0';
    if (ok)
    {
        /*
         * If we reached here via explain_mktemp_on_error, the original first
         * character has been restored.  We only need the following hack if
         * the user called mktemp directly.
         */
        first = pathname[0];
        if (first == '\0')
        {
            /*
             * Try to undo the brain-damage of the way mktemp(3) reports
             * errors.  Assume the programmer was at least half-way sane.  Our
             * first guess is that the NUL replaces a slash (assume /tmp or
             * similar), except in the limiting case, when it had to be an 'X'.
             */
            pathname[0] = '/';
            if (strlen(pathname) <= 6)
                pathname[0] = 'X';
        }
    }

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_mktemp_system_call(&exp.system_call_sb, errnum,
        pathname);
    explain_buffer_errno_mktemp_explanation(&exp.explanation_sb, errnum,
        "mktemp", pathname);
    explain_explanation_assemble(&exp, sb);

    /*
     * re-inflict the brain damage
     */
    if (ok)
        pathname[0] = first;
}


/* vim: set ts=8 sw=4 et : */
