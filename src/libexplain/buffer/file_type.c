/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h> /* for major()/minor() except Solaris */
#include <libexplain/ac/sys/sysmacros.h> /* for major()/minor() on Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/option.h>


void
explain_buffer_file_type(explain_string_buffer_t *sb, int mode)
{
    if (sb->maximum < FILE_TYPE_BUFFER_SIZE_MIN)
    {
        explain_string_buffer_printf
        (
            sb->footnotes,
            "; bug (file %s, line %d) explain_buffer_file_type buffer too "
                "small (%d < %d)",
            __FILE__,
            __LINE__,
            (int)sb->maximum,
            FILE_TYPE_BUFFER_SIZE_MIN
        );
    }

    mode &= S_IFMT;
    switch (mode)
    {
#ifdef __linux__
    case 0:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2) for
             * more information) when that file is a Linux kernel special file.
             */
            i18n("kernel special file")
        );
        break;
#endif

    case S_IFSOCK:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a network socket.
             */
            i18n("socket")
        );
        break;

    case S_IFLNK:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a symbolic link.
             */
            i18n("symbolic link")
        );
        break;

    case S_IFREG:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a regular file.
             */
            i18n("regular file")
        );
        break;

    case S_IFBLK:
        /* FIXME: on :inux, use /proc/devices to be more specific */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a block special
             * device.
             */
            i18n("block special device")
        );
        break;

    case S_IFDIR:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a directory.
             */
            i18n("directory")
        );
        break;

    case S_IFCHR:
        /* FIXME: on Linux, use /proc/devices to be more specific */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a character
             * special device.
             */
            i18n("character special device")
        );
        break;

    case S_IFIFO:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a fifo.
             */
            i18n("named pipe")
        );
        break;

#ifdef S_IFMPC
    case S_IFMPC:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a multiplexed
             * character special device.
             * Not present on all POSIX implementations.
             */
            i18n("multiplexed character special device")
        );
        break;
#endif

#ifdef S_IFNAM
    case S_IFNAM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a named special
             * file.  Not present on all POSIX implementations.
             */
            i18n("named special file")
        );
        /*
         * with two subtypes, distinguished by st_rdev values 1, 2
         * 0001   S_INSEM    s    000001   XENIX semaphore subtype of IFNAM
         * 0002   S_INSHD    m    000002   XENIX shared data subtype of IFNAM
         */
        break;
#endif

#ifdef S_IFMPB
    case S_IFMPB:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see
             * stat(2) for more information) when that file is a
             * multiplexed block special device.  Not present on all
             * POSIX implementations.
             */
            i18n("multiplexed block special device")
        );
        break;
#endif

#ifdef S_IFCMP
    case S_IFCMP:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a VxFS compressed
             * file.  Not present on all POSIX implementations.
             */
            i18n("VxFS compressed file")
        );
        break;
#endif

#ifdef S_IFNWK
    case S_IFNWK:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a network special
             * file.  Not present on all POSIX implementations.
             */
            i18n("network special file")
        );
        break;
#endif

#ifdef S_IFDOOR
    case S_IFDOOR:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a Solaris door.
             * Not present on all POSIX implementations.
             */
            i18n("Solaris door")
        );
        break;
#endif

#ifdef S_IFWHT
    case S_IFWHT:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is a BSD whiteout
             * file, used by the union file system.  Not present on all
             * POSIX implementations.
             */
            i18n("BSD whiteout")
        );
        break;
#endif

    default:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This string is the type of a file (see stat(2)
             * for more information) when that file is of an unknown
             * type, often the result of a bad inode block on a hard disk.
             */
            i18n("unknown file type")
        );
        explain_string_buffer_printf(sb, " (%#o)", mode);
        break;
    }
}


static int
proc_devices(char *buffer, size_t buffer_size, dev_t st_rdev, int pref)
{
    int             dev_major;
    FILE            *fp;
    char            line[100];

    if (buffer_size < 2)
        return 0;
    dev_major = major(st_rdev);
    fp = fopen("/proc/devices", "r");
    if (!fp)
        return 0;

    for (;;)
    {
        if (!fgets(line, sizeof(line), fp))
        {
            fclose(fp);
            return 0;
        }
        if (line[0] == pref)
            break;
    }
    for (;;)
    {
        char            *ep;
        long            n;

        if (!fgets(line, sizeof(line), fp) || line[0] == '\n')
        {
            fclose(fp);
            return 0;
        }
        ep = 0;
        n = strtol(line, &ep, 10);
        if (ep > line && n == dev_major)
        {
            while (*ep == ' ')
                ++ep;
            if (*ep != '/')
            {
                char *end = ep;
                while (*end && *end != '\n')
                    ++end;
                if (end > ep)
                {
                    size_t len = end - ep;
                    if (len > buffer_size - 1)
                        len = buffer_size - 1;
                    memcpy(buffer, ep, len);
                    buffer[len] = '\0';
                    fclose(fp);
                    return 1;
                }
            }
        }
    }
}


static int
proc_devices_blk(char *buffer, size_t buffer_size, dev_t st_rdev)
{
    return proc_devices(buffer, buffer_size, st_rdev, 'B');
}


static int
proc_devices_chr(char *buffer, size_t buffer_size, dev_t st_rdev)
{
    return proc_devices(buffer, buffer_size, st_rdev, 'C');
}


static int
usb_in_dev_symlink(const char *kind, dev_t st_rdev)
{
    int             n;
    char            path[100];
    char            slink[PATH_MAX];

    snprintf
    (
        path,
        sizeof(path),
        "/sys/dev/%s/%d:%d",
        kind,
        (int)major(st_rdev),
        (int)minor(st_rdev)
    );
    n = readlink(path, slink, sizeof(slink) - 1);
    if (n <= 0)
        return 0;
    slink[n] = '\0';
    return (0 != strstr(slink, "usb"));
}


void
explain_buffer_file_type_st(explain_string_buffer_t *sb, const struct stat *st)
{
    if (!st)
    {
        explain_buffer_file_type(sb, -1);
        return;
    }
    if (!explain_option_extra_device_info())
    {
        explain_buffer_file_type(sb, st->st_mode);
        return;
    }

    switch (st->st_mode & S_IFMT)
    {
    case S_IFBLK:
        {
            char            buffer[FILE_TYPE_BUFFER_SIZE_MIN];

            if (usb_in_dev_symlink("block", st->st_rdev))
            {
                explain_string_buffer_puts(sb, "usb ");
            }
            if (proc_devices_blk(buffer, sizeof(buffer), st->st_rdev))
            {
                explain_string_buffer_puts(sb, buffer);
                explain_string_buffer_putc(sb, ' ');
            }
            explain_buffer_file_type(sb, st->st_mode);
        }
        break;

    case S_IFCHR:
        {
            char            buffer[FILE_TYPE_BUFFER_SIZE_MIN];

            if (usb_in_dev_symlink("char", st->st_rdev))
            {
                explain_string_buffer_puts(sb, "usb ");
            }
            if (proc_devices_chr(buffer, sizeof(buffer), st->st_rdev))
            {
                explain_string_buffer_puts(sb, buffer);
                explain_string_buffer_putc(sb, ' ');
            }
            explain_buffer_file_type(sb, st->st_mode);
        }
        break;

#ifdef S_IFNAM
    case S_IFNAM:
        switch (st->st_rdev)
        {
        default:
            explain_buffer_file_type(sb, st->st_mode);
            break;

        case S_INSEM:
            /* FIXME: i18n */
            explain_string_buffer_puts(sb, "semaphore named special file");
            break;

        case S_INSHD:
            /* FIXME: i18n */
            explain_string_buffer_puts(sb, "shared data named special file");
            break;
        }
        break;
#endif

    default:
        explain_buffer_file_type(sb, st->st_mode);
        break;
    }
}


/* vim: set ts=8 sw=4 et : */
