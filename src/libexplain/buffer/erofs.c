/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/types.h> /* must be first */
#include <libexplain/ac/linux/cdrom.h>
#include <libexplain/ac/linux/fd.h>
#include <libexplain/ac/linux/major.h>
#include <libexplain/ac/linux/kdev_t.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/mtio.h>
#include <libexplain/ac/unistd.h> /* for ioctl() on Solaris */

#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>


static void
explain_buffer_erofs_generic(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext:  This error message is ised to explain an EROFS error,
         * usually from an open(2) system call.
         *
         * %1$s => The name of the offending system call argument
         */
        i18n("write access was requested and %s refers to a file on a "
            "read-only file system"),
        caption
    );
}


static void
explain_buffer_erofs_generic_dev(explain_string_buffer_t *sb,
    const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext:  This error message is ised to explain an EROFS error,
         * usually from an open(2) system call, in the case where write access
         * was requested for a read-only device.
         *
         * %1$s => The name of the offending system call argument
         */
        i18n("write access was requested and %s refers to a read-only "
            "device"),
        caption
    );
}


void
explain_buffer_erofs(explain_string_buffer_t *sb, const char *pathname,
    const char *caption)
{
    struct stat     st;

    if (stat(pathname, &st) >= 0)
    {
        switch (st.st_mode & S_IFMT)
        {
        case S_IFBLK:
        case S_IFCHR:
            explain_buffer_erofs_generic_dev(sb, caption);
            return;

        default:
            break;
        }
    }

    explain_buffer_erofs_generic(sb, caption);
    if (explain_buffer_mount_point(sb, pathname) < 0)
        explain_buffer_mount_point_dirname(sb, pathname);
}


void
explain_buffer_erofs_fildes(explain_string_buffer_t *sb, int fildes,
    const char *caption)
{
#ifdef CDROM_GET_CAPABILITY
    {
        int             cap;

        cap = 0;
        if (ioctl(fildes, CDROM_GET_CAPABILITY, &cap) >= 0)
        {
            /*
             * Device is a CD-ROM drive, or similar.
             *
             * Remember: in English it's is a "disc" with-a-C when it's
             * a CD or a DVD (blame the French, it started out as their
             * standard), and it's a "disk" with-a-K in every other case.
             */
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued to explain an
                 * EROFS error, in the case of a CD-ROM disc (or similar).
                 */
                i18n("the disc cannot be written to")
            );
            return;
        }
    }
#endif

#ifdef FDGETDRVTYP
    {
        floppy_drive_name fdt;

        if (ioctl(fildes, FDGETDRVTYP, &fdt) >= 0)
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued to explain an EROFS
                 * error, in the case of a floppy disk (or similar).
                 */
                i18n("the disk has the write-protect tab set")
            );
            return;
        }
    }
#endif

#ifdef MTIOCGET
    {
        struct mtget    status;

        if (ioctl(fildes, MTIOCGET, &status) >= 0)
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued to explain an EROFS
                 * error, in the case of a magnetic tape (or similar).
                 */
                i18n("the tape has the write-protect tab set")
            );
            return;
        }
    }
#endif

    /* check for devices */
    {
        struct stat     st;

        if (fstat(fildes, &st) >= 0)
        {
            switch (st.st_mode & S_IFMT)
            {
            case S_IFBLK:
#ifdef MMC_BLOCK_MAJOR
                /*
                 * Somewhat bizarrely, SD/MMC driver defines *no* ioctls.
                 */
                if (MAJOR(st.st_rdev) == MMC_BLOCK_MAJOR)
                {
                    explain_buffer_gettext
                    (
                        sb,
                        /*
                         * xgettext:  This error message is issued to explain
                         * an EROFS error, in the case of a Secure Disk /
                         * Multimedia Card.
                         */
                        i18n("the SD/MMC card has the write-protect tab set")
                    );
                    return;
                }
#endif
                /* fall through... */

            case S_IFCHR:
                explain_buffer_erofs_generic_dev(sb, caption);
                return;

            default:
                break;
            }
        }
    }

    explain_buffer_erofs_generic(sb, caption);
    explain_buffer_mount_point_fd(sb, fildes);
}


/* vim: set ts=8 sw=4 et : */
