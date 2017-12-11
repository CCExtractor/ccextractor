/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/fdopendir.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fdopendir_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fdopendir(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fdopendir_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes)
{
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    case ENOTDIR:
        explain_buffer_enotdir_fd(sb, fildes, "fildes");
        break;

    case EINVAL:
        {
            /* recapitulate some eglibc logic */
            int flags = fcntl(fildes, F_GETFL);
            if (flags >= 0)
            {
                /* FIXME: O_DIRECTORY */
                if ((flags & O_ACCMODE) == O_WRONLY)
                {
                    explain_buffer_ebadf_not_open_for_reading
                    (
                        sb,
                        "fildes",
                        flags
                    );
                    break;
                }
            }
        }
        /* fall through... */

    case ENOSYS:
    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_fdopendir(explain_string_buffer_t *sb, int errnum,
    int fildes)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fdopendir_system_call(&exp.system_call_sb, errnum,
        fildes);
    explain_buffer_errno_fdopendir_explanation(&exp.explanation_sb, errnum,
        "fdopendir", fildes);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
