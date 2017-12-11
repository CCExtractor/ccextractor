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

#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/emlink.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/link.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/exdev.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/note/still_exists.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/dirname.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
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
explain_buffer_errno_link_system_call(explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "link(oldpath = ");
    explain_buffer_pathname(sb, oldpath);
    explain_string_buffer_puts(sb, ", newpath = ");
    explain_buffer_pathname(sb, newpath);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_link_explanation(explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath)
{
    explain_final_t oldpath_final_component;
    explain_final_t newpath_final_component;

    explain_final_init(&oldpath_final_component);
    explain_final_init(&newpath_final_component);
    newpath_final_component.must_exist = 0;
    newpath_final_component.want_to_create = 1;
    newpath_final_component.st_mode = get_mode(oldpath);

    switch (errnum)
    {
    case EACCES:
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
             * Unable to pin point an exact cause, go with the generic
             * explanation.
             */
            explain_string_buffer_puts
            (
                sb,
                "write access to the directory containing newpath is "
                "denied, or search permission is denied for one of the "
                "directories in the path prefix of oldpath or newpath "
            );
        }
        break;

    case EEXIST:
        explain_buffer_eexist(sb, newpath);
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

    case EIO:
        explain_buffer_eio_path(sb, oldpath);
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

    case EPERM:
        {
            struct stat     oldpath_st;

            if (stat(oldpath, &oldpath_st) >= 0)
            {
                if (S_ISDIR(oldpath_st.st_mode))
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        "oldpath is a directory and it is not possible "
                        "to make hard links to directories; have you "
                        "considered using a symbolic link?"
                    );
                }
                else
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        "the file system containing oldpath and newpath "
                        "does not support the creation of hard links"
                    );
                }
            }
            else
            {
                /*
                 * Unable to pin point a specific cause,
                 * issue the generic explanation.
                 */
                explain_string_buffer_puts
                (
                    sb,
                    "oldpath is a directory; or, the file system "
                    "containing oldpath and newpath does not support "
                    "the creation of hard links"
                );
            }
        }
        break;

    case EROFS:
        explain_buffer_erofs(sb, oldpath, "oldpath");
        break;

    case EXDEV:
        explain_buffer_exdev(sb, oldpath, newpath, "link");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "link");
        break;
    }

    /*
     * Let the user know where things stand after the failure.
     */
    explain_buffer_note_if_exists(sb, newpath, "newpath");
}


void
explain_buffer_errno_link(explain_string_buffer_t *sb, int errnum,
    const char *oldpath, const char *newpath)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_link_system_call
    (
        &exp.system_call_sb,
        errnum,
        oldpath,
        newpath
    );
    explain_buffer_errno_link_explanation
    (
        &exp.explanation_sb,
        errnum,
        oldpath,
        newpath
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
