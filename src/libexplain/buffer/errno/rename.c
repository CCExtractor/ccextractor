/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/param.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/emlink.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/rename.h>
#include <libexplain/buffer/exdev.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/note/still_exists.h>
#include <libexplain/buffer/path_to_pid.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/capability.h>
#include <libexplain/count_directory_entries.h>
#include <libexplain/dirname.h>
#include <libexplain/explanation.h>
#include <libexplain/have_permission.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>
#include <libexplain/pathname_is_a_directory.h>
#include <libexplain/string_buffer.h>


static int
get_mode(const char *pathname)
{
    struct stat     st;

    if (stat(pathname, &st) < 0)
        return S_IFREG;
    return st.st_mode;
}


static void
explain_buffer_errno_rename_system_call(explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "rename(oldpath = ");
    explain_buffer_pathname(sb, oldpath);
    explain_string_buffer_puts(sb, ", newpath = ");
    explain_buffer_pathname(sb, newpath);
    explain_string_buffer_putc(sb, ')');
}


static void
dir_vs_not_dir(explain_string_buffer_t *sb, const char *dir_caption,
    const char *not_dir_caption, const struct stat *not_dir_st)
{
    explain_string_buffer_t ftype_sb;
    char            ftype[FILE_TYPE_BUFFER_SIZE_MIN];

    explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
    explain_buffer_file_type_st(&ftype_sb, not_dir_st);
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain an
         * EISDIR error reported by a rename(2) system call,
         * in the case where there is a file type mismatch.
         *
         * %1$s => the name of the source system call argument
         * %2$s => the name of the destination system call argument
         * %3$s => The file type of the destination,
         *         e.g. "regular file"
         */
        i18n("%s is a directory, but %s is a %s, not a directory"),
        dir_caption,
        not_dir_caption,
        ftype
    );
}


static void
dir_vs_not_dir2(explain_string_buffer_t *sb, const char *dir_caption,
    const char *not_dir_caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain an
         * EISDIR error reported by a rename(2) system call,
         * in the case where there is a file type mismatch,
         * but the precise file type of oldpath cannot be
         * determined.
         *
         * %1$s => the name of the source system call argument
         * %2$s => the name of the destination system call argument
         */
        i18n("%s is an existing directory, but %s is not a directory"),
        dir_caption,
        not_dir_caption
    );
}


static void
explain_buffer_errno_rename_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *oldpath,
    const char *newpath)
{
    explain_final_t oldpath_final_component;
    explain_final_t newpath_final_component;

    explain_final_init(&oldpath_final_component);
    oldpath_final_component.want_to_unlink = 1;
    oldpath_final_component.st_mode = get_mode(oldpath);
    explain_final_init(&newpath_final_component);
    newpath_final_component.must_exist = 0;
    newpath_final_component.want_to_create = 1;
    newpath_final_component.st_mode = oldpath_final_component.st_mode;

    switch (errnum)
    {
    case EACCES:
        /*
         * Check specific requirements for renaming directories.
         *
         * Note that this is not the case for Solaris.
         *
         * FIXME: how do we ask pathconf if we need this test?
         * It is possible that this next test does not apply for any file
         * system that does not actually store "." and/or ".." in the
         * directory data, but generates them at getdent/readdir time.
         */
#ifndef __sun__
        {
            struct stat     oldpath_st;

            /*
             * See if we have write permission on the oldpath containing
             * directory, so that we can update st_nlink for "."
             */
            if
            (
                stat(oldpath, &oldpath_st) >= 0
            &&
                S_ISDIR(oldpath_st.st_mode)
            &&
                !explain_have_write_permission
                (
                    &oldpath_st,
                    &oldpath_final_component.id
                )
            )
            {
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used to explan an
                     * EACCES error reported by a rename(2) system
                     * call.  This is the generic explanation given when
                     * renaming directories when path_resolution(7) is
                     * unable to provide a more specific explanation.
                     *
                     * %1$s => The name of the offending system call argument.
                     */
                    i18n("%s is a directory and does not allow write "
                        "permission, this is needed to update the \"..\" "
                        "directory entry"),
                    "oldpath"
                );
                break;
            }
        }
#endif

        /*
         * Check the paths themselves
         */
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                oldpath,
                "oldpath",
                &oldpath_final_component
            )
        &&
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                newpath,
                "newpath",
                &newpath_final_component
            )
        )
        {
            /*
             * Unable to find anything specific, give the generic
             * explanation.
             */
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explan an EACCES
                 * error reported by a rename(2) system call.  This is
                 * the generic explanation given when renaming things
                 * other than directories when path_resolution(7) is
                 * unable to provide a more specific explanation.
                 */
                i18n("write permission is denied for the directory "
                "containing oldpath or newpath; or, search permission "
                "is denied for one of the directory components of "
                "oldpath or newpath")
            );
        }
        break;

    case EBUSY:
        if (explain_buffer_path_users(sb, oldpath, "oldpath") > 0)
            break;
        if (explain_buffer_path_users(sb, newpath, "newpath") > 0)
            break;
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EBUSY error
             * reported by a rename(2) system call.  This is the generic
             * message given when a more specific explanation can not be
             * determined.
             */
            i18n("oldpath or newpath is a directory that is in use "
            "by some process (perhaps as current working directory, "
            "or as root directory, or it was open for reading) or is "
            "in use by the system (for example as a mount point)")
        );
        break;

    case EFAULT:
        if (explain_is_efault_path(oldpath))
        {
            explain_buffer_efault(sb, "oldpath");
            break;
        }
        if (explain_is_efault_path(newpath))
        {
            explain_buffer_efault(sb, "newpath");
            break;
        }
        explain_buffer_efault(sb, "oldpath or newpath");
        break;

    case EINVAL:
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINVAL error
             * reported by a rename(2) system call, in the case where an
             * attempt was made to make a directory a subdirectory of
             * itself
             *
             * %1$s => the name of the source system call argument
             * %2$s => the name of the destination system call argument
             */
            i18n("%s contained a path prefix of %s; or, "
                "more generally, an attempt was made to make a directory a "
                "subdirectory of itself"),
            "newpath",
            "oldpath"
        );
        break;

    case EISDIR:
        {
            struct stat     oldpath_st;

            if (lstat(oldpath, &oldpath_st) >= 0)
            {
                dir_vs_not_dir(sb, "newpath", "oldpath", &oldpath_st);
            }
            else
            {
                dir_vs_not_dir2(sb, "newpath", "oldpath");
            }
        }
        break;

    case ELOOP:
        explain_buffer_eloop2
        (
            sb,
            oldpath,
            "oldpath",
            &oldpath_final_component,
            newpath,
            "newpath",
            &newpath_final_component
        );
        break;

    case EMLINK:
        explain_buffer_emlink(sb, oldpath, newpath);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong2
        (
            sb,
            oldpath,
            "oldpath",
            &oldpath_final_component,
            newpath,
            "newpath",
            &newpath_final_component
        );
        break;

    case ENOENT:
        explain_buffer_enoent2
        (
            sb,
            oldpath,
            "oldpath",
            &oldpath_final_component,
            newpath,
            "newpath",
            &newpath_final_component
        );
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSPC:
        explain_buffer_enospc(sb, newpath, "newpath");
        break;

    case ENOTDIR:
        if (explain_pathname_is_a_directory(oldpath))
        {
            struct stat     newpath_st;

            if
            (
                stat(newpath, &newpath_st) >= 0
            &&
                !S_ISDIR(newpath_st.st_mode)
            )
            {
                dir_vs_not_dir(sb, "oldpath", "newpath", &newpath_st);
                break;
            }
        }

        explain_buffer_enotdir2
        (
            sb,
            oldpath,
            "oldpath",
            &oldpath_final_component,
            newpath,
            "newpath",
            &newpath_final_component
        );
        break;

    case ENOTEMPTY:
    case EEXIST:
        {
            int             count;

            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain and
                 * ENOTEMPTY or EEXIST error reported by a rename(2)
                 * system call, in the case where both oldpath and
                 * newpath are directpries, but newpath is not empty.
                 *
                 * %1$s => the name of the offending system call argument
                 */
                i18n("%s is not an empty directory; that is, it "
                    "contains entries other than \".\" and \"..\""),
                "newpath"
            );

            count = explain_count_directory_entries(newpath);
            if (count > 0)
                explain_string_buffer_printf(sb, " (%d)", count);
        }
        break;

    case EPERM:
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errno,
                oldpath,
                "oldpath",
                &oldpath_final_component
            )
        &&
            explain_buffer_errno_path_resolution
            (
                sb,
                errno,
                newpath,
                "newpath",
                &newpath_final_component
            )
        )
        {
            /* FIXME: this needs to be much more specific */
            explain_buffer_eperm_unlink(sb, oldpath, "oldpath", syscall_name);
            explain_string_buffer_puts(sb, "; or, ");
            explain_buffer_eperm_unlink(sb, newpath, "newpath", syscall_name);
        }
        break;

    case EROFS:
        explain_buffer_erofs(sb, newpath, "newpath");
        break;

    case EXDEV:
        explain_buffer_exdev(sb, oldpath, newpath, syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }

    /*
     * Let the user know where things stand after the failure.
     */
    explain_buffer_note_if_still_exists(sb, oldpath, "oldpath");
    explain_buffer_note_if_exists(sb, newpath, "newpath");
}


void
explain_buffer_errno_rename(explain_string_buffer_t *sb, int errnum,
    const char *oldpath, const char *newpath)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_rename_system_call
    (
        &exp.system_call_sb,
        errnum,
        oldpath,
        newpath
    );
    explain_buffer_errno_rename_explanation
    (
        &exp.explanation_sb,
        errnum,
        "rename",
        oldpath,
        newpath
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
