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

#include <libexplain/ac/acl/libacl.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/acl.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h> /* pathconf */

#include <libexplain/buffer/acl.h>
#include <libexplain/buffer/acl_type.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/acl_set_file.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acl_set_file_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, acl_type_t type, acl_t acl)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_set_file(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_acl_type(sb, type);
    explain_string_buffer_puts(sb, ", acl = ");
    explain_buffer_acl(sb, acl);
    explain_string_buffer_putc(sb, ')');
}


static int
is_a_directory(const char*pathname)
{
    struct stat st;
    if (stat(pathname, &st) < 0)
        return 0;
    return !!S_ISDIR(st.st_mode);
}


void
explain_buffer_errno_acl_set_file_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname, acl_type_t type,
    acl_t acl)
{
    explain_final_t final_component;

    (void)type;
    explain_final_init(&final_component);
    final_component.want_to_modify_inode = 1;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acl_set_file.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        explain_buffer_efault(sb, "pathname");
        break;

    case EACCES:
        if (type == ACL_TYPE_DEFAULT && !is_a_directory(pathname))
        {
            /*
             * Note undocumented, discovered by reading the libacl source code
             */
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "The type argument is ACL_TYPE_DEFAULT, but the file "
                "referred to by pathname is not a directory"
            );
            break;
        }

        /*
         * Search permission is denied for a component of the
         * path prefix or the object exists and the process does
         * not have appropriate access rights.
         *
         * Argument type specifies a type of ACL that cannot be
         * associated with pathname.
         */
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EINVAL:
        if (!acl)
        {
            explain_buffer_is_the_null_pointer(sb, "acl");
            break;
        }
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        /*
         * The argument acl does not point to a valid ACL.
         */
        if (acl_valid(acl) < 0)
        {
            /* FIXME: i18n */
            explain_string_buffer_puts(sb, "acl does not point to a valid ACL");
            break;
        }
        if (type == ACL_TYPE_DEFAULT && !is_a_directory(pathname))
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "The type argument is ACL_TYPE_DEFAULT, but the file "
                "referred to by pathname is not a directory"
            );
            break;
        }

        if
        (
            type != ACL_TYPE_ACCESS
        &&
            type != ACL_TYPE_DEFAULT
#ifdef AC_TYPE_EXTENDED
        &&
            type != ACL_TYPE_EXTENDED
#endif
#ifdef AC_TYPE_NFSv4
        &&
            type != ACL_TYPE_NFSv4
#endif
        )
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the type parameter is not correctly specified"
            );
            break;
        }

        {
            long num = acl_entries(acl);
#ifdef _PC_ACL_PATH_MAX
            long max = pathconf(pathname, _PC_ACL_PATH_MAX);
#else
            long max = -1;
#endif
            if (num > max)
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "The ACL has more entries than the file referred to "
                    "by pathname can obtain "
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
        goto generic;

    case ENAMETOOLONG:
        /*
         * The length of the argument pathname is too long.
         */
        explain_buffer_enametoolong(sb, pathname, "pathname", &final_component);
        break;

    case ENOENT:
        /*
         * The named object does not exist or the argument pathname
         * points to an empty string.
         */
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOSPC:
        /*
         * The directory or file system that would contain the
         * new ACL cannot be extended or the file system is out
         * of file allocation resources.
         */
        explain_buffer_enospc(sb, pathname, "pathnme");
        break;

    case ENOTDIR:
        /*
         * A component of the path prefix is not a directory.
         */
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case ENOTSUP:
    case ENOSYS:
#if defined(EOPNOTSUP) && ENOSYS != EOPNOTSUP
    case EOPNOTSUP:
#endif
        /*
         * The file identified by pathname cannot be associated
         * with the ACL because the file system on which the file
         * is located does not support this.
         *  or ACLs are disabled.
         * or this host does not support ACLs.
         */
        explain_buffer_enosys_acl(sb, "pathname", syscall_name);
        break;

    case EPERM:
        /*
         * The process does not have appropriate privilege to
         * perform the operation to set the ACL.
         */
        if
        (
            /* returns zero if prints somthing */
            !explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                pathname,
                "pathname",
                &final_component
            )
        )
            break;

        explain_buffer_eperm_vague(sb, syscall_name);
        explain_buffer_dac_sys_admin(sb);
        break;

    case EROFS:
        /*
         * This function requires modification of a file system
         * which is currently read-only.
         */
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_set_file(explain_string_buffer_t *sb, int errnum,
    const char *pathname, acl_type_t type, acl_t acl)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_set_file_system_call(&exp.system_call_sb, errnum,
        pathname, type, acl);
    explain_buffer_errno_acl_set_file_explanation(&exp.explanation_sb, errnum,
        "acl_set_file", pathname, type, acl);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
