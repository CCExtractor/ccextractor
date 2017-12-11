/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/buffer/errno/lseek.h>
#include <libexplain/buffer/errno/pread.h>
#include <libexplain/buffer/errno/read.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_pread_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, void *data, size_t data_size, off_t offset)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "pread(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_puts(sb, ", offset = ");
    explain_buffer_off_t(sb, offset);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_pread_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int fildes, void *data, size_t data_size,
    off_t offset)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/pread.html
     */
    switch (errnum)
    {
    default:
        caused_by_read:
        explain_buffer_errno_read_explanation
        (
            sb,
            errnum,
            syscall_name,
            fildes,
            data,
            data_size
        );
        break;

    case EBADF:
    case ESPIPE:
    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
        caused_by_lseek:
        explain_buffer_errno_lseek_explanation
        (
            sb,
            errnum,
            syscall_name,
            fildes,
            offset,
            SEEK_SET
        );
        break;

    case EINVAL:
        if (offset < 0)
            goto caused_by_lseek;
        goto caused_by_read;
    }
}


void
explain_buffer_errno_pread(explain_string_buffer_t *sb, int errnum, int fildes,
    void *data, size_t data_size, off_t offset)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_pread_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        data,
        data_size,
        offset
    );
    explain_buffer_errno_pread_explanation
    (
        &exp.explanation_sb,
        errnum,
        "pread",
        fildes,
        data,
        data_size,
        offset
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
