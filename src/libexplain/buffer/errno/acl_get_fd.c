/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/acl_get_fd.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acl_get_fd_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_get_fd(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_acl_get_fd_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int fildes)
{
    /* acl_type_t type = ACL_TYPE_ACCESS; */

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acl_get_fd.html
     */
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ENOMEM:
        /*
         * The ACL working storage requires more memory than is
         * allowed by the hardware or system-imposed memory management
         * constraints.
         */
        explain_buffer_enomem_user(sb, 0);
        break;

    case ENOTSUP:
    case ENOSYS:
#if defined(EOPNOTSUP) && ENOSYS != EOPNOTSUP
    case EOPNOTSUP:
#endif
        explain_buffer_enosys_acl(sb, "fildes", syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_get_fd(explain_string_buffer_t *sb, int errnum, int
    fildes)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_get_fd_system_call(&exp.system_call_sb, errnum,
        fildes);
    explain_buffer_errno_acl_get_fd_explanation(&exp.explanation_sb, errnum,
        "acl_get_fd", fildes);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
