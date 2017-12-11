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
#include <libexplain/ac/acl/libacl.h>

#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/acl.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/acl_set_fd.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acl_set_fd_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, acl_t acl)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_set_fd(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", acl = ");
    explain_buffer_acl(sb, acl);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_acl_set_fd_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, acl_t acl)
{
    /* acl_type_t type = ACL_TYPE_ACCESS; */

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acl_set_fd.html
     */
    switch (errnum)
    {
    case EBADF:
        /*
         * The fildes argument is not a valid file descriptor.
         */
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EINVAL:
        if (!acl)
        {
            explain_buffer_is_the_null_pointer(sb, "acl");
            break;
        }
        if (acl_valid(acl) < 0)
        {
            /*
             * The argument acl does not point to a valid ACL.
             */
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "The argument %s does not point to a valid ACL",
                "acl"
            );
            break;
        }

        /*
         * The ACL has more entries than the file referred to by fildes
         * can obtain.
         */
        {
            long num = acl_entries(acl);
#ifdef _PC_ACL_PATH_MAX
            long max = pathconf(pathname, _PC_ACL_PATH_MAX);
#else
            long max = -1;
#endif
            if (num > max)
            {
                explain_string_buffer_printf
                (
                    sb,
                    /* FIXME: i18n */
                    ("The ACL has more entries than the file referred to "
                    "by %s can obtain"),
                    "fildes"
                );
#ifdef _PC_ACL_PATH_MAX
                if (explain_option_dialect_specific())
                {
                    explain_string_buffer_printf(" (%d > %d)", num, max);
                }
#endif
                break;
            }
        }

        explain_buffer_einval_vague(sb, "acl");
        break;

    case ENOSPC:
        /*
         * The directory or file system that would contain the new
         * ACL cannot be extended or the file system is out of file
         * allocation resources.
         */
        explain_buffer_enospc_fildes(sb, fildes, "fildes");
        break;

    case ENOSYS:
    case ENOTSUP:
#if defined(EOPNOTSUP) && ENOSYS != EOPNOTSUP
    case EOPNOTSUP:
#endif
        /*
         * The file identified by fildes cannot be associated with the ACL
         * because the file system on which the file is located does not
         * support this.
         */
        explain_buffer_enosys_acl(sb, "fildes", syscall_name);
        break;

    case EPERM:
        /*
         * The process does not have appropriate privilege to perform
         * the operation to set the ACL.
         */
        explain_buffer_does_not_have_inode_modify_permission_fd
        (
            sb,
            fildes,
            "fildes"
        );
        break;

    case EROFS:
        /*
         * This function requires modification of a file system which is
         * currently read-only.
         */
        explain_buffer_erofs_fildes(sb, fildes, "fildes");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_set_fd(explain_string_buffer_t *sb, int errnum,
    int fildes, acl_t acl)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_set_fd_system_call(&exp.system_call_sb, errnum,
        fildes, acl);
    explain_buffer_errno_acl_set_fd_explanation(&exp.explanation_sb, errnum,
        "acl_set_fd", fildes, acl);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
