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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/stat.h> /* for major()/minor() except Solaris */
#include <libexplain/ac/sys/sysmacros.h> /* for major()/minor() on Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eisdir.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomedium.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/etxtbsy.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/path_to_pid.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/pretty_size.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/capability.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_open_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int flags, int mode)
{
    explain_string_buffer_printf(sb, "open(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_open_flags(sb, flags);
    if (flags & O_CREAT)
    {
        explain_string_buffer_puts(sb, ", mode = ");
        explain_buffer_permission_mode(sb, mode);
    }
    explain_string_buffer_putc(sb, ')');
}


static void
you_can_not_open_a_socket(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This message is used when open(2) received an
         * ENODEV error, and the pathname it attempted to open was a
         * socket (first character "s" is ls(1) long output).  They
         * probably meant to use a named pipe (first character "p" in
         * ls(1) long outout).
         */
        i18n("you cannot use open(2) to open socket files, you must use "
        "connect(2) instead; a named pipe may be what was intended")
    );
}


static void
no_corresponding_device(explain_string_buffer_t *sb, const struct stat *st)
{
    char            ftype[FILE_TYPE_BUFFER_SIZE_MIN];
    char            numbers[40];
    explain_string_buffer_t ftype_sb;
    explain_string_buffer_t numbers_sb;

    explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
    explain_buffer_file_type_st(&ftype_sb, st);

    explain_string_buffer_init(&numbers_sb, numbers, sizeof(numbers));
    explain_string_buffer_printf
    (
        &numbers_sb,
        "(%ld, %ld)",
        (long)major(st->st_dev),
        (long)minor(st->st_dev)
    );
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain an ENODEV error reported by
         * an open(2) system call, and the device does not actually exist.
         *
         * %1$s => the file type of the special file,
         *         already translated.
         * %2$s => the major and minor device numbers
         *
         * Example: "pathname refers to a block special file (42, 13) and no
         *           corresponding device exists"
         */
        i18n("pathname refers to a %s %s and no corresponding device exists"),
        ftype,
        numbers
    );
}


static int
is_device_file(const struct stat *st)
{
    switch (st->st_mode & S_IFMT)
    {
    case S_IFCHR:
    case S_IFBLK:
#ifdef S_IFMPC
    case S_IFMPC:
#endif
#ifdef S_IFMPB
   case S_IFMPB:
#endif
        return 1;

    default:
        break;
    }
    return 0;
}


void
explain_buffer_errno_open_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname, int flags,
    int mode)
{
    explain_final_t final_component;

    (void)mode;
    explain_final_init(&final_component);
    switch (flags & O_ACCMODE)
    {
    case O_RDONLY:
        final_component.want_to_read = 1;
        break;

    case O_RDWR:
        final_component.want_to_read = 1;
        final_component.want_to_write = 1;
        break;

    case O_WRONLY:
        final_component.want_to_write = 1;
        break;

    default:
        assert(!"unknown open access mode");
        break;
    }
    if (flags & O_CREAT)
    {
        final_component.want_to_create = 1;
        final_component.must_exist = 0;
        if (flags & O_EXCL)
            final_component.must_not_exist = 1;
    }
    if (flags & O_DIRECTORY)
    {
        final_component.must_be_a_st_mode = 1;
        final_component.st_mode = S_IFDIR;
    }
    if (flags & O_NOFOLLOW)
        final_component.follow_symlink = 0;

    switch (errnum)
    {
    case EACCES:
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                pathname,
                "pathname",
                &final_component
            )
        )
        {
            struct stat     st;

            if
            (
                stat(pathname, &st) >= 0
            &&
                is_device_file(&st)
            &&
                explain_mount_point_nodev(&st)
            )
            {
                explain_string_buffer_t file_type_sb;
                char            file_type[FILE_TYPE_BUFFER_SIZE_MIN];

                explain_string_buffer_init(&file_type_sb, file_type,
                    sizeof(file_type));
                explain_buffer_file_type_st(&file_type_sb, &st);
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext:  This message is used when explaining an EACCES
                     * error returned by an open(2) system call, in the case
                     * where the file is a character special device or a block
                     * special device, and the file system has been mounted with
                     * the "nodev" option.
                     *
                     * %1$s => the file type (character sepcial device, etc)
                     *         already translated.
                     */
                    i18n("the %s is on a file system mounted with the "
                        "\"nodev\" option"),
                    file_type
                );
                explain_buffer_mount_point_stat(sb, &st);
                break;
            }

            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This message is used when explaining an
                 * EACCES error returned by an open(2) system call.
                 * Usually path_resolution(7) will have a better
                 * explanation, this explanation is only used when a
                 * more specific explanation is not available.
                 */
                i18n("the requested access to the file is not allowed, or "
                "search permission is denied for one of the directories "
                "in the path prefix of pathname, or the file did not "
                "exist yet and write access to the parent directory is "
                "not allowed")
            );
        }
        break;

    case EINVAL:
        explain_buffer_einval_bits(sb, "flags");
        break;

    case EEXIST:
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                pathname,
                "pathname",
                &final_component
            )
        )
        {
            explain_buffer_eexist(sb, pathname);
        }
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

#if defined(O_LARGEFILE) && (O_LARGEFILE != 0)
    case EFBIG:
    case EOVERFLOW:
        if (!(flags & O_LARGEFILE))
        {
            struct stat     st;

            if (stat(pathname, &st) >= 0 && S_ISREG(st.st_mode))
            {
                char            siz[20];
                explain_string_buffer_t siz_sb;

                explain_string_buffer_init(&siz_sb, siz, sizeof(siz));
                {
                    explain_string_buffer_putc(sb, '(');
                    explain_buffer_pretty_size(sb, st.st_size);
                    explain_string_buffer_putc(sb, ')');
                }
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext:  This message is used to explain an EFBIG
                     * or EOVERFLOW error reported by and open(2) system
                     * call.  The file is, in fact, too large to be opened
                     * without the O_LARGEFILE flag.
                     *
                     * %1$s => The size of the file, in parentheses
                     */
                    i18n("pathname referes to a regular file that is too large "
                        "to be opened %s, the O_LARGEFILE flag is necessary"),
                    siz
                );
            }
        }
        break;
#endif

    case EISDIR:
        if ((flags & O_ACCMODE) != O_RDONLY)
        {
            if (explain_buffer_eisdir(sb, pathname, "pathname"))
                break;
        }
        goto generic;

    case ELOOP:
    case EMLINK: /* BSD */
        if (flags & O_NOFOLLOW)
        {
            struct stat     st;

            if (lstat(pathname, &st) >= 0 && S_ISLNK(st.st_mode))
            {
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used to explain an
                     * ELOOP or EMLINK error reported by an open(2)
                     * system call, in the case where the O_NOFOLLOW
                     * flags was specified but the final path component
                     * was a symbolic link.
                     */
                    i18n("O_NOFOLLOW was specified but pathname refers to a "
                    "symbolic link")
                );
                /* FIXME: mention this may indicate some kind of attack? */
                break;
            }
        }
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong
        (
            sb,
            pathname,
            "pathname",
            &final_component
        );
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

#ifdef ENOMEDIUM
    case ENOMEDIUM:
        explain_buffer_enomedium(sb, pathname);
        break;
#endif

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSPC:
        explain_buffer_enospc(sb, pathname, "pathname");
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case ENXIO:
        {
            struct stat     st;

            if (stat(pathname, &st) < 0)
            {
                enxio_generic:
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext:  This message is used to explain an
                     * ENXIO error returned by an open(2) system call.
                     * This is the generic explanation, used when no
                     * more specific cause can be determined.
                     */
                    i18n("O_NONBLOCK | O_WRONLY is set, the named file "
                    "is a FIFO and no process has the file open for "
                    "reading; or, the file is a device special file and "
                    "no corresponding device exists")
                );
                break;
            }
            switch (st.st_mode & S_IFMT)
            {
            case S_IFIFO:
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used to explain an
                     * ENXIO error returned by an open(2) system call,
                     * in the case where a named pipe has no readers,
                     * and a non-blocking writer tried to open it.
                     */
                    i18n("O_NONBLOCK | O_WRONLY is set, and the named file "
                    "is a FIFO, and no process has the file open for "
                    "reading")
                );
                /* FIXME: what happens if you open a named pipe O_RDWR? */
                break;

            case S_IFCHR:
            case S_IFBLK:
                no_corresponding_device(sb, &st);
                break;

            case S_IFSOCK:
                you_can_not_open_a_socket(sb);
                break;

            default:
                goto enxio_generic;
            }
        }
        break;

#if defined(O_NOATIME) && (O_NOATIME != 0)
    case EPERM:
        if (flags & O_NOATIME)
        {
            struct stat     st;
            explain_string_buffer_t puid_sb;
            explain_string_buffer_t ftype_sb;
            explain_string_buffer_t fuid_sb;
            char            puid[100];
            char            ftype[FILE_TYPE_BUFFER_SIZE_MIN];
            char            fuid[100];

            explain_string_buffer_init(&puid_sb, puid, sizeof(puid));
            explain_buffer_uid(&puid_sb, geteuid());
            explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
            explain_string_buffer_init(&fuid_sb, fuid, sizeof(fuid));
            if (stat(pathname, &st) >= 0)
            {
                explain_buffer_file_type_st(&ftype_sb, &st);
                explain_buffer_uid(&fuid_sb, st.st_uid);
            }
            else
                explain_buffer_file_type(&ftype_sb, S_IFREG);
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when an EPERM erro is
                 * returned by an open(2) system call, and the O_NOATIME
                 * open flag was specified, but the process lacked the
                 * permissions required.
                 *
                 * %1$s => the number and name of the process effective UID,
                 *         already quoted if needed
                 * %2$s => the file type of the file in question,
                 *         almost always "regular file" (already translated)
                 * %3$s => the number and name of the file owner UID,
                 *         already quoted if needed
                 */
                i18n("the O_NOATIME flags was specified, but the process "
                "effective UID %s does not match the %s owner UID %s"),
                puid,
                ftype,
                fuid
            );

            /*
             * also explain the necessary priviledge
             */
            explain_buffer_dac_fowner(sb);
        }
        break;
#endif

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    case ETXTBSY:
        explain_buffer_etxtbsy(sb, pathname);
        break;

    case EWOULDBLOCK:
        if (flags & O_NONBLOCK)
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an
                 * EWOULDBLOCK error returned by an open(2) system call,
                 * when the use of thr O_NONBLOCK flags would otherwise
                 * cause the open(2) system call to block.
                 */
                i18n("the O_NONBLOCK flag was specified, and an "
                "incompatible lease was held on the file")
            );

            /*
             * Look for other processes with this file open,
             * and list their PIDs.
             */
            explain_buffer_path_to_pid(sb, pathname);
        }
        break;

    case ENODEV:
        {
            struct stat     st;

            if (stat(pathname, &st) >= 0)
            {
                switch (st.st_mode & S_IFMT)
                {
                case S_IFSOCK:
                    you_can_not_open_a_socket(sb);
                    break;

                case S_IFBLK:
                case S_IFCHR:
                    no_corresponding_device(sb, &st);
#ifdef __linux__
                    if (explain_option_dialect_specific())
                    {
                        explain_string_buffer_puts(sb->footnotes, "; ");
                        explain_buffer_gettext
                        (
                            sb->footnotes,
                            /*
                             * xgettext: This message is used to explain an
                             * ENODEV error reported by an open(2) system
                             * call, which shoudl actually have been a ENXIO
                             * error instead.  They are easy to confuse,
                             * they have exactly the same English text
                             * returned from strerror(3).
                             */
                            i18n("this is a Linux kernel bug, in this "
                            "situation POSIX says ENXIO should have been "
                            "returned")
                        );
                    }
#endif
                    break;

                default:
                    break;
                }
            }
        }
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }

    if (flags & O_TRUNC)
    {
        if ((flags & O_ACCMODE) == O_RDONLY)
        {
            explain_string_buffer_puts(sb->footnotes, "; ");
            explain_buffer_gettext
            (
                sb->footnotes,
                /*
                 * xgettext: This message is used to supplement an explanation
                 * for an error reported by open(2) system call, in the case
                 * where the caller used a flags combination with explicitly
                 * undefined behavior.
                 */
                i18n("note that the behavior of (O_RDONLY | O_TRUNC) is "
                    "undefined")
            );
        }
        else
        {
            struct stat st;

            if (stat(pathname, &st) >= 0)
            {
                explain_string_buffer_t file_type_sb;
                char            file_type[FILE_TYPE_BUFFER_SIZE_MIN];

                explain_string_buffer_init(&file_type_sb, file_type,
                    sizeof(file_type));
                explain_buffer_file_type_st(&file_type_sb, &st);

                switch (st.st_mode & S_IFMT)
                {
                default:
                    break;

                case S_IFSOCK:
                case S_IFIFO:
                    /* explicitly ignored */
                    {
                        explain_string_buffer_puts(sb->footnotes, "; ");
                        explain_string_buffer_printf_gettext
                        (
                            sb->footnotes,
                            /*
                             * xgettext: This message is used to supplement an
                             * explanation for an error reported by open(2)
                             * system call, in the case where the caller used
                             * O_TRUNC in a combination that explicitly
                             * ignores O_TRUNC.
                             *
                             * %1$s => the type of the special file, already
                             *         translated
                             */
                            i18n("note that a %s will ignore the O_TRUNC flag"),
                            file_type
                        );
                    }
                    break;

                case S_IFCHR:
                    /*
                     * The O_TRUNC flags is explicitly ignored for TTY devices,
                     * but is undefined for all other character devices.  Since
                     * we can't tell which case we have at this moment, we fall
                     * through...
                     */

                case S_IFBLK:
#ifdef S_IFMPC
                case S_IFMPC:
#endif
#ifdef S_IFMPB
                case S_IFMPB:
#endif
                    /* explicitly undefined */
                    {
                        explain_string_buffer_puts(sb->footnotes, "; ");
                        explain_string_buffer_printf_gettext
                        (
                            sb->footnotes,
                            /*
                             * xgettext: This message is used to supplement an
                             * explanation for an error reported by open(2)
                             * system call, in the case where the caller used
                             * O_TRUNC in a combination with explicitly
                             * undefined behavior.
                             *
                             * %1$s => the type of the special file, already
                             *         translated
                             */
                            i18n("note that the behavior of O_TRUNC on a %s is "
                                "undefined"),
                            file_type
                        );
                    }
                    break;
                }
            }
        }
    }

    if ((flags & O_EXCL) && !(flags & O_CREAT))
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        /* FIXME: except for a TTY, where it means single open */
        explain_buffer_gettext
        (
            sb->footnotes,
            /*
             * xgettext: This message is used to supplement an explanation for
             * an error reported by open(2) system call, and the caller used a
             * flags combination with explicitly undefined behavior.
             */
            i18n("note that the behavior of O_EXCL is undefined if "
            "O_CREAT is not specified")
        );
    }
}


void
explain_buffer_errno_open(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int flags, int mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_open_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        flags,
        mode
    );
    explain_buffer_errno_open_explanation
    (
        &exp.explanation_sb,
        errnum,
        "open",
        pathname,
        flags,
        mode
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
