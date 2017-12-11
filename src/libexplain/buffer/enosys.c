/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/device_name.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_enosys_generic(explain_string_buffer_t *sb, const char *caption,
    const char *syscall_name)
{
    explain_string_buffer_printf
    (
        sb,
        /*
         * xgettext:  This error message is issued to explain an ENOSYS
         * or EOPNOTSUPP or ENOTTY error, in the generic case.  There are
         * more specific messages, try to use those instead.
         *
         * %1$s => the name of the offending system call argument
         * %2$s => the name of the offending system call
         */
        i18n("%s is not associated with an object to which %s can be applied"),
        caption,
        syscall_name
    );
}


static void
the_device_does_not_support_the_system_call(explain_string_buffer_t *sb,
    const char *file_type, const char *caption, const char *syscall_name)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: this error message is issued to explain an ENOSYS
         * or EOPNOTSUPP error in the case where a system call is not
         * supported for a particular device (or perhapse si not
         * supported by the device driver).
         *
         * %1$s => the type of the special file (already translated)
         * %2$s => the name of the offending system call.
         */
        i18n("%s is a %s that does not support the %s system call"),
        caption,
        file_type,
        syscall_name
    );
}


static void
explain_buffer_enosys_stat(explain_string_buffer_t *sb, const struct stat *st,
    const char *caption, const char *syscall_name)
{
    switch (st->st_mode & S_IFMT)
    {
    case S_IFREG:
    case S_IFDIR:
    case S_IFLNK:
        {
            char            mount_point[PATH_MAX + 1];
            explain_string_buffer_t mount_point_buf;

            explain_string_buffer_init
            (
                &mount_point_buf,
                mount_point,
                sizeof(mount_point)
            );
            if (explain_buffer_mount_point_stat(&mount_point_buf, st) >= 0)
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: this error message is issued to explain
                     * an ENOSYS or EOPNOTSUPP error in the case where a
                     * file system does not support a particular system
                     * call.
                     *
                     * %1$s => the mount point of the file system,
                     *         in parentheses
                     * %2$s => the name of the offending system call.
                     */
                    i18n("the file system %s does not support the %s system "
                        "call"),
                    mount_point,
                    syscall_name
                );
            }
            else
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: this error message is issued to explain
                     * an ENOSYS or EOPNOTSUPP error in the case where a
                     * file system does not support a particular system
                     * call.
                     *
                     * %1$s => the name of the offending system call.
                     */
                    i18n("the file system does not support the %s system call"),
                    syscall_name
                );
            }
        }
        break;

    case S_IFBLK:
    case S_IFCHR:
        {
            struct stat     st2;
            char            device_name[150];
            explain_string_buffer_t device_name_buf;
            char            file_type[FILE_TYPE_BUFFER_SIZE_MIN];
            explain_string_buffer_t file_type_buf;

            explain_string_buffer_init
            (
                &file_type_buf,
                file_type,
                sizeof(file_type)
            );
            explain_buffer_file_type_st(&file_type_buf, st);
            explain_string_buffer_init
            (
                &device_name_buf,
                device_name,
                sizeof(device_name)
            );
            if
            (
                explain_buffer_device_name(&device_name_buf, st->st_dev, &st2)
            >=
                0
            )
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: this error message is issued to explain
                     * an ENOSYS or EOPNOTSUPP error in the case where
                     * a system call is not supported for a particular
                     * device (or perhapse not supported by the device
                     * driver).
                     *
                     * %1$s => the file system path of the device special file
                     * %2$s => the type of the special file (already translated)
                     * %3$s => the name of the offending system call.
                     */
                    i18n("the %s %s does not support the %s system call"),
                    device_name,
                    file_type,
                    syscall_name
                );
            }
            else
            {
                the_device_does_not_support_the_system_call
                (
                    sb,
                    file_type,
                    caption,
                    syscall_name
                );
            }
        }
        break;

    default:
        {
            char            file_type[FILE_TYPE_BUFFER_SIZE_MIN];
            explain_string_buffer_t file_type_buf;

            explain_string_buffer_init
            (
                &file_type_buf,
                file_type,
                sizeof(file_type)
            );
            explain_buffer_file_type_st(&file_type_buf, st);
            the_device_does_not_support_the_system_call
            (
                sb,
                file_type,
                caption,
                syscall_name
            );
        }
        break;
    }
}


void
explain_buffer_enosys_fildes(explain_string_buffer_t *sb, int fildes,
    const char *caption, const char *syscall_name)
{
    struct stat     st;

    if (fstat(fildes, &st) >= 0)
        explain_buffer_enosys_stat(sb, &st, caption, syscall_name);
    else
        explain_buffer_enosys_generic(sb, caption, syscall_name);
}


void
explain_buffer_enosys_pathname(explain_string_buffer_t *sb,
    const char *pathname, const char *caption, const char *syscall_name)
{
    struct stat     st;

    if (stat(pathname, &st) >= 0)
        explain_buffer_enosys_stat(sb, &st, caption, syscall_name);
    else
        explain_buffer_enosys_generic(sb, caption, syscall_name);
}


void
explain_buffer_enosys_acl(explain_string_buffer_t *sb, const char *caption,
    const char *syscall_name)
{
    explain_string_buffer_printf
    (
        sb,
        i18n
        (
            /*
             * xgettext:  This error message is issued to explain an ENOSYS
             * or EOPNOTSUPP or ENOTSUP error, when returned by one of the
             * ACL functions.
             *
             * %1$s => the name of the offending system call argument
             * %2$s => the name of the offending system call
             */
            "the %s argument is not associated with an object to which the "
            "%s system call can be applied, or the file system on which the "
            "file is located may not support ACLs, or ACLs are disabled, or "
            "this host system does not support ACLs"
        ),
        caption,
        syscall_name
    );
}


/* vim: set ts=8 sw=4 et : */
