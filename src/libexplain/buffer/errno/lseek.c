/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2013 Peter Miller
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
#include <libexplain/ac/stdio.h> /* for snprintf */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h> /* for fpathconf */

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/lseek.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/lseek_whence.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_lseek_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, off_t offset, int whence)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "lseek(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", offset = ");
    explain_buffer_off_t(sb, offset);
    explain_string_buffer_puts(sb, ", whence = ");
    explain_buffer_lseek_whence(sb, whence);
    explain_string_buffer_putc(sb, ')');
}


#if defined(SEEK_HOLE) || defined(SEEK_DATA)

static int
holes_are_supported(int fildes)
{
#ifdef _PC_MIN_HOLE_SIZE
    long            result;

    errno = 0;
    result = fpathconf(fildes, _PC_MIN_HOLE_SIZE);
    if (result == -1 && errno != 0)
    {
        switch (errno)
        {
        case EINVAL:
        case ENOSYS:
            /* holes are not supported */
            return 0;

        case EBADF:
        default:
            /* can't tell */
            return -1;
        }
    }
    if (result <= 0)
    {
        /* holes are not supported */
        return 0;
    }
    /* holes are supported */
    return 1;
#else
    /* can't tell */
    (void)fildes;
    return -1;
#endif
}

#endif


void
explain_buffer_errno_lseek_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, off_t offset, int whence)
{
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EINVAL:
        {
            long long       current_position;
            long long       destination;

            current_position = lseek(fildes, 0, SEEK_CUR);
            switch (whence)
            {
            default:
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "'whence' is not one of SEEK_SET, SEEK_CUR, SEEK_END"
#ifdef SEEK_HOLE
                    ", SEEK_HOLE"
#endif
#ifdef SEEK_DATA
                    ", SEEK_DATA"
#endif
                );
                return;

#ifdef SEEK_HOLE
            case SEEK_HOLE:
                if (!holes_are_supported(fildes))
                    goto seek_hole_enosys;
                if (offset < 0)
                {
                    explain_buffer_einval_too_small(sb, "offset", offset);
                    return;
                }
                destination = lseek(fildes, offset, whence);
                offset = 0;
                break;
#endif

#ifdef SEEK_DATA
            case SEEK_DATA:
                if (!holes_are_supported(fildes))
                    goto seek_data_enosys;
                if (offset < 0)
                {
                    explain_buffer_einval_too_small(sb, "offset", offset);
                    return;
                }
                destination = lseek(fildes, offset, whence);
                offset = 0;
                break;
#endif

            case SEEK_SET:
                current_position = 0;
                destination = offset;
                break;

            case SEEK_CUR:
                destination = current_position + offset;
                break;

            case SEEK_END:
                destination = lseek(fildes, 0, SEEK_END);
                if (destination < 0)
                {
                    current_position = -1;
                    destination = 0;
                }
                else
                {
                    current_position = 0;
                    destination += offset;
                }
                break;
            }

            if (current_position != (off_t)-1)
            {
                if (destination < 0)
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        /* FIXME: i18n */
                        "the resulting file offset would be "
                        "negative"
                    );
                }
                else
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        /* FIXME: i18n */
                        "the resulting offset would be beyond "
                        "the end of a seekable device"
                    );
                }
                explain_string_buffer_printf
                (
                    sb,
                    " (%lld)",
                    (long long)destination
                );
                break;
            }

            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the resulting file offset would be negative, "
                "or beyond the end of a seekable device"
            );
        }
        break;

    case ENXIO:
#ifdef SEEK_HOLE
        if (whence == SEEK_HOLE)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "there is no hole extent beyond the current file position"
            );
        }
#endif
#ifdef SEEK_DATA
        if (whence == SEEK_DATA)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "there is no data extent beyond the current file position"
            );
        }
#endif
        goto generic;

    case EOVERFLOW:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the resulting file offset cannot be represented in an off_t"
        );
        break;

    case ESPIPE:
        {
            struct stat     st;

            if (fstat(fildes, &st) == 0)
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "the file descriptor is associated with a "
                );
                explain_buffer_file_type_st(sb, &st);
            }
            else
            {
                explain_string_buffer_puts
                (
                    sb,
                    /* FIXME: i18n */
                    "the file descriptor is associated with a "
                    "pipe, socket, or FIFO"
                );
            }
        }
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
#ifdef SEEK_HOLE
        if (whence == SEEK_HOLE && !holes_are_supported(fildes))
        {
            char            temp[50];

            seek_hole_enosys:
            snprintf(temp, sizeof(temp), "%s SEEK_HOLE", syscall_name);
            explain_buffer_enosys_fildes(sb, fildes, "fildes", temp);
            break;
        }
#endif
#ifdef SEEK_DATA
        if (whence == SEEK_DATA && !holes_are_supported(fildes))
        {
            char            temp[50];

            seek_data_enosys:
            snprintf(temp, sizeof(temp), "%s SEEK_DATA", syscall_name);
            explain_buffer_enosys_fildes(sb, fildes, "fildes", temp);
            break;
        }
#endif
        explain_buffer_enosys_fildes(sb, fildes, "fildes", syscall_name);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_lseek(explain_string_buffer_t *sb, int errnum,
    int fildes, off_t offset, int whence)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_lseek_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        offset,
        whence
    );
    explain_buffer_errno_lseek_explanation
    (
        &exp.explanation_sb,
        errnum,
        "lseek",
        fildes,
        offset,
        whence
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
