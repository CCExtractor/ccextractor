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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/errno/close.h>
#include <libexplain/buffer/errno/fclose.h>
#include <libexplain/buffer/errno/write.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/stream.h>
#include <libexplain/buffer/note/underlying_fildes_open.h>
#include <libexplain/explanation.h>
#include <libexplain/stream_to_fildes.h>


static void
explain_buffer_errno_fclose_system_call(explain_string_buffer_t *sb,
    int errnum, FILE *fp)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fclose(fp = ");
    explain_buffer_stream(sb, fp);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fclose_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, FILE *fp)
{
    int             fildes;

    if (!fp)
    {
        explain_buffer_is_the_null_pointer(sb, "fp");
        return;
    }

    /*
     * The Linux fclose(3) man page says
     *
     *     "RETURN VALUE:  Upon successful completion 0 is returned.
     *     Otherwise, EOF is returned and the global variable errno is
     *     set to indicate the error.  In either case any further access
     *     (including another call to fclose()) to the stream results in
     *     undefined behavior."
     *
     * which is interesting because if close(2) fails, the file
     * descriptor is usually still open.  Thus, we make an attempt
     * to recover the file descriptor, to see if we can produce some
     * additional information.
     *
     * If you are using glibc you are plain out of luck, because
     * it very carefully assigns -1 to the file descriptor member.
     * Other implementations may not be so careful, indeed other
     * implementations may keep the FILE pointer valid if the underlying
     * file descriptor is still valid.
     */
    fildes = explain_stream_to_fildes(fp);

    switch (errnum)
    {
    case EFAULT:
    case EFBIG:
    case EINVAL:
    case ENOSPC:
    case EPIPE:
        explain_buffer_errno_write_explanation
        (
            sb,
            errnum,
            syscall_name,
            fildes,
            NULL,
            0
        );
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fp");
        break;

    case EINTR:
    case EIO:
    default:
        explain_buffer_errno_close_explanation
        (
            sb,
            errnum,
            syscall_name,
            fildes
        );
        break;
    }
    if (errnum != EBADF)
    {
        explain_buffer_note_underlying_fildes_open(sb);
    }
}


void
explain_buffer_errno_fclose(explain_string_buffer_t *sb, int errnum,
    FILE *fp)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fclose_system_call(&exp.system_call_sb, errnum, fp);
    explain_buffer_errno_fclose_explanation
    (
        &exp.explanation_sb,
        errnum,
        "fclose",
        fp
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
