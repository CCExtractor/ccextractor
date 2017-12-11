/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/linux/cdrom.h>
#include <libexplain/ac/linux/fd.h>
#include <libexplain/ac/linux/kdev_t.h>
#include <libexplain/ac/linux/major.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/mtio.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/enomedium.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/sizeof.h>


#ifdef FDGETDRVTYP

static const char *
translate_drive_type(const char *code_name)
{
    typedef struct table_t table_t;
    struct table_t
    {
        const char *code_name;
        const char *human_readable;
    };

    static const table_t table[] =
    {
        { "(null)", "" },
        { "d360", "360KB PC" },
        { "h1200", "1.2MB AT" },
        { "D360", "360KB SS 3.5\"" },
        { "D720", "720KB 3.5\"" },
        { "h360", "360KB AT" },
        { "h720", "720KB AT" },
        { "H1440", "1.44MB 3.5\"" },
        { "E2880", "2.88MB 3.5\"" },
        { "E3120", "3.12MB 3.5\"" },
        { "h1440", "1.44MB 5.25\"" },
        { "H1680", "1.68MB 3.5\"" },
        { "h410", "410KB 5.25\"" },
        { "H820", "820KB 3.5\"" },
        { "h1476", "1.48MB 5.25\"" },
        { "H1722", "1.72MB 3.5\"" },
        { "h420", "420KB 5.25\"" },
        { "H830", "830KB 3.5\"" },
        { "h1494", "1.49MB 5.25\"" },
        { "H1743", "1.74 MB 3.5\"" },
        { "h880", "880KB 5.25\"" },
        { "D1040", "1.04MB 3.5\"" },
        { "D1120", "1.12MB 3.5\"" },
        { "h1600", "1.6MB 5.25\"" },
        { "H1760", "1.76MB 3.5\"" },
        { "H1920", "1.92MB 3.5\"" },
        { "E3200", "3.20MB 3.5\"" },
        { "E3520", "3.52MB 3.5\"" },
        { "E3840", "3.84MB 3.5\"" },
        { "H1840", "1.84MB 3.5\"" },
        { "D800", "800KB 3.5\"" },
        { "H1600", "1.6MB 3.5\"" },
    };

    const table_t   *tp;

    for (tp = table; tp < ENDOF(table); ++tp)
    {
        if (0 == strcmp(code_name, tp->code_name))
            return tp->human_readable;
    }
    return code_name;
}

#endif
#ifdef MMC_BLOCK_MAJOR

static void
explain_buffer_enomedium_sdmmc(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This error message is issued to explain an ENOMEDIUM
         * error, in the case of a SD/MMC slot (or similar).
         */
        i18n("there does not appear to be a card in the SD/MMC slot")
    );
}

#endif

static int
explain_buffer_enomedium_fildes_q(explain_string_buffer_t *sb, int fildes)
{
#ifdef CDROM_GET_CAPABILITY
    {
        int             cap;

        cap = 0;
        if (ioctl(fildes, CDROM_GET_CAPABILITY, &cap) >= 0)
        {
            const char      *drive_type;

            /*
             * FIXME: we could be even more informative, if the
             * CDROM_DRIVE_STATUS ioctl is supported:
             *     CDS_NO_INFO if not implemented
             *     CDS_NO_DISC
             *     CDS_TRAY_OPEN
             */
            drive_type = "CD-ROM";
            if (cap & (CDC_DVD | CDC_DVD_R | CDC_DVD_RAM))
                drive_type = "DVD";

            /*
             * Definitely a CD-ROM drive, or similar.
             *
             * Remember: in English it's is a "disc" with-a-C when it's
             * a CD or a DVD (blame the French, it started out as their
             * standard), and it's a "disk" with-a-K in every other case.
             */
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued to explain an
                 * ENOMEDIUM error, in the case of a CD-ROM drive (or
                 * similar).
                 *
                 * %1$s => the type of drive, "CD-ROM" or "DVD".
                 */
                i18n("there does not appear to be a disc in the %s drive"),
                drive_type
            );
            return 0;
        }
    }
#endif

#ifdef FDGETDRVTYP
    {
        floppy_drive_name fdt;

        if (ioctl(fildes, FDGETDRVTYP, &fdt) >= 0)
        {
            const char      *drive_type;

            drive_type = translate_drive_type(fdt);
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued to explain an
                 * ENOMEDIUM error, in the case of a floppy drive (or
                 * similar).
                 *
                 * %1$s => the type of floppy drive, e.g. "3 1/2"
                 */
                i18n("there does not appear to be a disk in the %s floppy "
                    "drive"),
                drive_type
            );
            return 0;
        }
    }
#endif

#ifdef MTIOCGET
    {
        struct mtget    status;

        if (ioctl(fildes, MTIOCGET, &status) >= 0)
        {
#if defined(MT_ISSCSI1) || defined(MT_ISSCSI2)
            switch (status.mt_type)
            {
#ifdef MT_ISSCSI1
            case MT_ISSCSI1:
#endif
#ifdef MT_ISSCSI2
            case MT_ISSCSI2:
#endif
                /*
                 * FIXME: could we get more information into the type name?
                 * Particularly if it is a SCSI device, the mode pages will have
                 * more information.
                 */
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext:  This error message is issued to explain an
                     * ENOMEDIUM error, in the case of a SCSI tape drive
                     * (or similar).
                     */
                    i18n("there does not appear to be a tape in the SCSI tape "
                        "drive")
                );
                break;

            default:
#endif
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext:  This error message is issued to explain an
                     * ENOMEDIUM error, in the case of a magnetic tape drive
                     * (or similar).
                     */
                    i18n("there does not appear to be a tape in the magnetic "
                        "tape drive")
                );
#if defined(MT_ISSCSI1) || defined(MT_ISSCSI2)
                break;
            }
#endif
            return 0;
        }
    }
#endif

#ifdef MMC_BLOCK_MAJOR
    {
        struct stat     st;

        if
        (
            fstat(fildes, &st) >= 0
        &&
            S_ISBLK(st.st_mode)
        &&
            MAJOR(st.st_rdev) == MMC_BLOCK_MAJOR
        )
        {
            explain_buffer_enomedium_sdmmc(sb);
            return 0;
        }
    }
#endif

    /*
     * report that we failed
     */
    return -1;
}


void
explain_buffer_enomedium_fildes(explain_string_buffer_t *sb, int fildes)
{
    if (explain_buffer_enomedium_fildes_q(sb, fildes))
        explain_buffer_enomedium_generic(sb);
}


void
explain_buffer_enomedium(explain_string_buffer_t *sb, const char *pathname)
{
#if defined(CDROM_GET_CAPABILITY) || defined(FDGETDRVTYP) || defined(MTIOCGET)
    int             fildes;

    /*
     * On modern versions of Linux, if you open a CD-ROM drive with the
     * O_NONBLOCK flag, it works even when there is no disc present.
     * This permits all sorts of diagnostic information to be accessed.
     *
     * The same trick works for magnetic tapes.
     *
     * The linux kernel source reads as if this will work for floppy
     * disks, too, but I don't have one to experiment with any more.
     *
     * Note: this does NOT work with Linux SD/MMC device driver(s).
     */
    fildes = open(pathname, O_RDONLY + O_NONBLOCK);
    if (fildes >= 0)
    {
        if (explain_buffer_enomedium_fildes_q(sb, fildes) >= 0)
        {
            close(fildes);
            return;
        }

        /*
         * No luck, use the generic explanation.
         */
        close(fildes);
    }
#endif

#ifdef MMC_BLOCK_MAJOR
    {
        struct stat     st;

        if
        (
            stat(pathname, &st) >= 0
        &&
            S_ISBLK(st.st_mode)
        &&
            MAJOR(st.st_rdev) == MMC_BLOCK_MAJOR
        )
        {
            explain_buffer_enomedium_sdmmc(sb);
            return;
        }
    }
#endif

    explain_buffer_enomedium_generic(sb);
}


void
explain_buffer_enomedium_generic(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This error message is issued to explain an
         * ENOMEDIUM error, when a more specific explaination is not
         * available.
         */
        i18n("the disk drive is a type that has removable disks, and there "
        "does not appear to be a disk in the drive")
    );
}


/* vim: set ts=8 sw=4 et : */
