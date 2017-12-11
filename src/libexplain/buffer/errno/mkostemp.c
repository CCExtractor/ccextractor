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
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/mkostemp.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_mkostemp_system_call(explain_string_buffer_t *sb, int
    errnum, const char *pathname, int flags)
{
    (void)errnum;
    /*
     * the argument must be called "pathname" so that the explanations
     * for open() make sense.
     */
    explain_string_buffer_puts(sb, "mkostemp(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_open_flags(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_mkostemp_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, const char *pathname, int flags)
{
    size_t          len;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/mkostemp.html
     */
    switch (errnum)
    {
    case EINVAL:
        /*
         * This case could be ambiguous.  The flags combinations could
         * be invalid, or the template could be invalid.  Since the
         * template could have been modified at this point, we can only
         * detect the length being wrong.  Otherwise, assume the flags
         * bits were invalid.
         */
        len = strlen(pathname);
        if (len < 6)
        {
            explain_buffer_einval_mkstemp(sb, pathname, "pathname");
            break;
        }
        /* Fall through... */

    case EEXIST:
        explain_buffer_eexist_tempname_dirname(sb, pathname);
        break;

    default:
        explain_buffer_errno_open_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            /* this is what [e]glibc does to the flags */
            (flags & ~O_ACCMODE) | O_RDWR | O_CREAT | O_EXCL,
            S_IRUSR | S_IWUSR
        );
        break;
    }
}


void
explain_buffer_errno_mkostemp(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_mkostemp_system_call(&exp.system_call_sb, errnum,
        pathname, flags);
    explain_buffer_errno_mkostemp_explanation(&exp.explanation_sb, errnum,
        "mkostemp", pathname, flags);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
