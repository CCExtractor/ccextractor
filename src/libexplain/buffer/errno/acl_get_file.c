/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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

#include <libexplain/buffer/acl_type.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/acl_get_file.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_acl_get_file_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, acl_type_t type)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acl_get_file(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_acl_type(sb, type);
    explain_string_buffer_putc(sb, ')');
}


void explain_buffer_errno_acl_get_file_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    acl_type_t type)
{
    explain_final_t final_component;

    (void)type;
    explain_final_init(&final_component);
    final_component.want_to_modify_inode = 0;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acl_get_file.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        if (explain_is_efault_path(pathname))
        {
            explain_buffer_efault(sb, "pathname");
            break;
        }
        goto oops;

    case EACCES:
        /*
         * search permission is denied for a component of the
         * path prefix or the object exists and the process does
         * not have appropriate access rights.
         *
         * Argument type specifies a type of ACL that cannot be
         * associated with pathname.
         */
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EINVAL:
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        explain_string_buffer_printf
        (
            sb,
            /* FIXME: i18n */
            "the %s argument is not a known ACL type",
            "type"
        );
        explain_buffer_software_error(sb);
        break;

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

    case ENOMEM:
        /*
         * The ACL working storage requires more memory than is
         * allowed by the hardware or system-imposed memory management
         * constraints.
         */
        explain_buffer_enomem_user(sb, 0);
        break;

    case ENOTDIR:
        /*
         * A component of the path prefix is not a directory.
         */
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case ELOOP:
    case EMLINK: /* BSD */
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENOTSUP:
    case ENOSYS:
#if defined(EOPNOTSUP) && ENOSYS != EOPNOTSUP
    case EOPNOTSUP:
#endif
        /*
         * The file system on which the file identified by pathname
         * is located does not support ACLs, or ACLs are disabled.
         */
        explain_buffer_enosys_acl(sb, "pathname", syscall_name);
        break;

    default:
        oops:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_acl_get_file(explain_string_buffer_t *sb, int errnum, const
    char *pathname, acl_type_t type)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acl_get_file_system_call(&exp.system_call_sb, errnum,
        pathname, type);
    explain_buffer_errno_acl_get_file_explanation(&exp.explanation_sb, errnum,
        "acl_get_file", pathname, type);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
