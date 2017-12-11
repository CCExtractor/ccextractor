/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efbig.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/ftruncate.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/etxtbsy.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/fildes_not_open_for_writing.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_ftruncate_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, off_t length)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "ftruncate(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", length = ");
    explain_buffer_off_t(sb, length);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_ftruncate_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, off_t length)
{
    switch (errnum)
    {
    case EACCES:
    case EBADF:
        if (explain_buffer_fildes_not_open_for_writing(sb, fildes, "fildes"))
            explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFBIG:
        explain_buffer_efbig_fildes(sb, fildes, length, "length");
        break;

    case EINTR:
        explain_buffer_eintr(sb, "ftruncate");
        break;

    case EINVAL:
        if (length < 0)
        {
            /* FIXME: i18n */
            explain_string_buffer_puts(sb, "'length' is negative");
        }
        else
        {
#ifdef _PC_FILESIZEBITS
            long            file_size_bits;

            /*
             * FIXME: also use getrlimit(RLIMIT_FSIZE)
             *
             * RLIMIT_FSIZE
             *     The maximum size of files that the process may
             *     create.  Attempts to extend a file beyond this limit
             *     result in delivery of a SIGXFSZ signal.  By default,
             *     this signal terminates a process, but a process can
             *     catch this signal instead, in which case the relevant
             *     system call (e.g., write(2), truncate(2)) fails with
             *     the error EFBIG.
             */
            file_size_bits = fpathconf(fildes, _PC_FILESIZEBITS);
            if
            (
                file_size_bits > 0
            &&
                (size_t)file_size_bits < 8 * sizeof(off_t)
            &&
                length > (1LL << file_size_bits)
            )
            {
                explain_string_buffer_printf
                (
                    sb,
                    /* FIXME: i18n */
                    "'length' is larger than the maximum file size (2 ** %ld)",
                    file_size_bits
                );
            }
            else
#endif
            {
                int             flags;

                /* FIXME: i18n */
                flags = fcntl(fildes, F_GETFL);
                if (flags >= 0 && (flags & O_ACCMODE) == O_RDONLY)
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        "the file is not open for writing ("
                    );
                    explain_buffer_open_flags(sb, flags);
                    explain_string_buffer_putc(sb, ')');
                }
                else
                {
                    struct stat     st;

                    if (fstat(fildes, &st) >= 0 && !S_ISREG(st.st_mode))
                    {
                        explain_string_buffer_puts
                        (
                            sb,
                            "fildes refers to a "
                        );
                        explain_buffer_file_type_st(sb, &st);
                        explain_string_buffer_puts
                        (
                            sb,
                            ", it is only possible to truncate a "
                        );
                        explain_buffer_file_type(sb, S_IFREG);
                    }
                    else
                    {
                        explain_string_buffer_puts
                        (
                            sb,
                            "the file is not open for writing; or, the file is "
                            "not a "
                        );
                        explain_buffer_file_type(sb, S_IFREG);
                    }
                }
            }
        }
        break;

    case EIO:
        explain_buffer_eio_fildes(sb, fildes);
        break;

    case EISDIR:
        /* FIXME: i18n */
        explain_string_buffer_puts
        (
            sb,
            "fildes refers to a "
        );
        explain_buffer_file_type(sb, S_IFDIR);
        explain_string_buffer_puts
        (
            sb,
            ", it is only possible to truncate a "
        );
        explain_buffer_file_type(sb, S_IFREG);
        break;

    case EPERM:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the underlying file system does not support extending a "
            "file beyond its current size"
        );
        explain_buffer_mount_point_fd(sb, fildes);
        break;

    case EROFS:
        explain_buffer_erofs_fildes(sb, fildes, "fildes");
        break;

    case ETXTBSY:
        explain_buffer_etxtbsy_fildes(sb, fildes);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "ftruncate");
        break;
    }
}


void
explain_buffer_errno_ftruncate(explain_string_buffer_t *sb, int errnum,
    int fildes, off_t length)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_ftruncate_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        length
    );
    explain_buffer_errno_ftruncate_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        length
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
