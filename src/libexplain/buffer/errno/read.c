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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/mtio.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/read.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>
#include <libexplain/is_same_inode.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_read_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, const void *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "read(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static int
is_a_tape(int fildes)
{
    struct mtop     args;

    args.mt_op = MTNOP;
    args.mt_count = 0;
    return (ioctl(fildes, MTIOCTOP, &args) >= 0);
}


void
explain_buffer_errno_read_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int fildes, const void *data, size_t data_size)
{
    (void)data;
    (void)data_size;
    switch (errnum)
    {
    case EAGAIN:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "non-blocking I/O has been selected using "
            "O_NONBLOCK and no data was immediately available for "
            "reading"
        );
        break;

    case EBADF:
        {
            int             flags;

            flags = fcntl(fildes, F_GETFL);
            if (flags >= 0)
            {
                explain_buffer_ebadf_not_open_for_reading(sb, "fildes", flags);
            }
            else
            {
                explain_buffer_ebadf(sb, fildes, "fildes");
            }
        }
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        {
            int             flags;

            flags = fcntl(fildes, F_GETFL);
            if (flags >= 0)
            {
                if ((flags & O_ACCMODE) == O_WRONLY)
                {
                    explain_buffer_ebadf_not_open_for_reading
                    (
                        sb,
                        "fildes",
                        flags
                    );
                }
#ifdef O_DIRECT
                else if (flags & O_DIRECT)
                {
                    /* FIXME: figure out which */
                    explain_string_buffer_puts
                    (
                        sb,
                        /* FIXME: i18n */
                        "the file was opened with the O_DIRECT flag, "
                        "and either the address specified in data is "
                        "not suitably aligned, or the value specified "
                        "in data_size is not suitably aligned, or the "
                        "current file offset is not suitably aligned"
                    );
                }
#endif
                else
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        /* FIXME: i18n */
                        "the file descriptor was created via a call to "
                        "timerfd_create(2) and the wrong size buffer was "
                        "given"
                    );
                }
            }
            else
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "the file desriptor is attached to an object "
                    "which is unsuitable for reading; or, the file was "
                    "opened with the O_DIRECT flag, and either the "
                    "address specified in data, the value specified "
                    "in data_size, or the current file offset is not"
                    "suitably aligned; or, the file descriptor was "
                    "created via a call to timerfd_create(2) and the "
                    "wrong size buffer was given"
                );
            }
        }

        /*
         * There is a bug in Tru64 5.1.  Attempting to read more than
         * INT_MAX bytes fails with errno == EINVAL.
         */
        if (data_size > INT_MAX)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "you have tripped over a bug in Tru64 5.1 where it is unable "
                "to read more than INT_MAX bytes at a time"
            );
            explain_string_buffer_printf
            (
                sb,
                " (%zd > %d)",
                data_size,
                INT_MAX
            );
            break;
        }
        break;

    case EIO:
        {
            pid_t process_group = getpgid(0);
            int controlling_tty_fd = open("/dev/tty", O_RDWR);
            pid_t tty_process_group = tcgetpgrp(controlling_tty_fd);

            /* if reading controlling tty */
            if
            (
                process_group >= 0
            &&
                controlling_tty_fd >= 0
            &&
                process_group != tty_process_group
            )
            {
                struct stat     st1;
                struct stat     st2;

                if
                (
                    fstat(fildes, &st1) == 0
                &&
                    fstat(controlling_tty_fd, &st2) == 0
                &&
                    explain_is_same_inode(&st1, &st2)
                )
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        /* FIXME: i18n */
                        "the process is in a background process group, and "
                        "tried to read from its controlling tty, and the "
                        "controlling tty is either ignoring or blocking SIGTTIN"
                    );
                    close(controlling_tty_fd);
                    break;
                }
            }
            if (controlling_tty_fd < 0)
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "the process is in an orphaned process "
                    "group and tried to read from its controlling tty"
                );
                break;
            }
            close(controlling_tty_fd);

            explain_buffer_eio_fildes(sb, fildes);
        }
        break;

    case EISDIR:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "fildes refers to a directory, and you must use getdents(2) to "
            "read directories, preferably via the higher-level interface "
            "provided by readdir(3)"
        );
        break;

    case ENOENT:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the file is on a file system"
        );
        explain_buffer_mount_point_fd(sb, fildes);
        explain_string_buffer_puts
        (
            sb,
            " that does not support Unix open file semantics, and the "
            "file has been deleted from underneath you"
        );
        break;

    case EOVERFLOW:
        if (data_size > ((size_t)1 << 16) && is_a_tape(fildes))
        {
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "the tape read operation was supplied with a %ld byte "
                    "buffer, however the kernal has been compiled with a limit "
                    "smaller than this; you need to reconfigure your system, "
                    "or recompile your tape device driver, to have a fixed "
                    "limit of at least 64KiB",
                (long)data_size
            );
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_read(explain_string_buffer_t *sb, int errnum,
    int fildes, const void *data, size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_read_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        data,
        data_size
    );
    explain_buffer_errno_read_explanation
    (
        &exp.explanation_sb,
        errnum,
        "read",
        fildes,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
