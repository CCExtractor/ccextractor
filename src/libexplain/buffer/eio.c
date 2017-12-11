/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/dirent.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on solaris */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/ac/sys/mtio.h>
#ifdef __linux__
#include <libexplain/ac/linux/hdreg.h>
#endif
#include <libexplain/ac/termios.h>

#include <libexplain/buffer/device_name.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/dirname.h>


static void
possibly_as_a_result_of_a_preceding(explain_string_buffer_t *sb, int fildes)
{
    int             flags;

    flags = fcntl(fildes, F_GETFL);
    if (flags < 0)
        flags = O_RDWR;
    explain_string_buffer_puts(sb, ", ");
    switch (flags & O_ACCMODE)
    {
    case O_RDONLY:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EIO
             * error, for a file open only for reading.
             */
            i18n("possibly as a result of a preceding read(2) system call")
        );
        break;

    case O_WRONLY:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EIO
             * error, for a file open only for writing.
             */
            i18n("possibly as a result of a preceding write(2) system call")
        );
        break;

    default:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EIO
             * error, for a file open for both reading and writing.
             */
            i18n("possibly as a result of a preceding read(2) or "
            "write(2) system call")
        );
        break;
    }
}


static void
explain_buffer_eio_generic(explain_string_buffer_t *sb, int fildes)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EIO error.
         * Such errors are usually related to the underlying hardware of
         * the device being used, or the special device that contains
         * the file system the file is stored in.
         */
        i18n("a low-level I/O error occurred, probably in hardware")
    );
    possibly_as_a_result_of_a_preceding(sb, fildes);
}


void
explain_buffer_eio(explain_string_buffer_t *sb)
{
    explain_buffer_eio_generic(sb, -1);
}


static int
dev_stat(dev_t dev, struct stat *st, explain_string_buffer_t *dev_buf)
{
    return explain_buffer_device_name(dev_buf, dev, st);
}


static void
a_low_level_io_error_occurred(explain_string_buffer_t *sb,
    const char *device_path, const struct stat *st)
{
    char            ftype[FILE_TYPE_BUFFER_SIZE_MIN];
    explain_string_buffer_t ftype_sb;

    explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
    if (device_path[0])
    {
        explain_string_buffer_puts_quoted(&ftype_sb, device_path);
        explain_string_buffer_putc(&ftype_sb, ' ');
    }
    explain_buffer_file_type_st(&ftype_sb, st);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EIO error.
         * Such errors are usually related to the underlying hardware of
         * the device being used, or the special device that contains
         * the file system the file is stored in.
         *
         * %1$s => The device named and device's file type
         */
        i18n("a low-level I/O error occurred in the %s"),
        ftype
    );
}


static void
explain_buffer_eio_stat(explain_string_buffer_t *sb, int fildes,
    struct stat *st)
{
    char            dev_path[150];
    explain_string_buffer_t dev_buf;

    explain_string_buffer_init(&dev_buf, dev_path, sizeof(dev_path));
    assert(dev_path[0] == '\0');
    switch (st->st_mode & S_IFMT)
    {
    case S_IFDIR:
    case S_IFREG:
        if (dev_stat(st->st_dev, st, &dev_buf) < 0)
        {
            explain_buffer_eio(sb);
            return;
        }
        break;

    default:
        dev_stat(st->st_rdev, st, &dev_buf);
        /* no problem if it failed, use the st we were given */
        break;
    }

    switch (st->st_mode & S_IFMT)
    {
    case S_IFBLK:
    case S_IFCHR:
        a_low_level_io_error_occurred(sb, dev_path, st);
        possibly_as_a_result_of_a_preceding(sb, fildes);
        break;

    default:
        explain_buffer_eio_generic(sb, fildes);
        break;
    }
}


void
explain_buffer_eio_fildes(explain_string_buffer_t *sb, int fildes)
{
    struct stat     st;

    if (fstat(fildes, &st) < 0)
        explain_buffer_eio(sb);
    else
        explain_buffer_eio_stat(sb, fildes, &st);
}


void
explain_buffer_eio_path(explain_string_buffer_t *sb, const char *path)
{
    struct stat     st;

    if (stat(path, &st) < 0 && lstat(path, &st) < 0)
        explain_buffer_eio(sb);
    else
        explain_buffer_eio_stat(sb, -1, &st);
}


void
explain_buffer_eio_path_dirname(explain_string_buffer_t *sb,
    const char *path)
{
    char            path_dir[PATH_MAX + 1];

    explain_dirname(path_dir, path, sizeof(path_dir));
    explain_buffer_eio_path(sb, path_dir);
}


/* vim: set ts=8 sw=4 et : */
