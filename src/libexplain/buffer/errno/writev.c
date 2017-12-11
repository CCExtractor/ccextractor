/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/sys/uio.h>
#include <libexplain/ac/unistd.h> /* for sysconf() on Solaris */

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/write.h>
#include <libexplain/buffer/errno/writev.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/iovec.h>
#include <libexplain/buffer/long_long.h>
#include <libexplain/buffer/ssize_t.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_writev_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, const struct iovec *data, int data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "writev(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_iovec(sb, data, data_size);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_int(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_writev_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int fildes, const struct iovec *data,
    int data_size)
{
    void            *data2;
    size_t          data2_size;
    int             j;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/writev.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (explain_is_efault_pointer(data, data_size * sizeof(*data)))
        {
            explain_buffer_efault(sb, "data");
            return;
        }
        for (j = 0; j < data_size; ++j)
        {
            const struct iovec *p = data + j;
            if (explain_is_efault_pointer(p->iov_base, p->iov_len))
            {
                char            buffer[60];

                snprintf(buffer, sizeof(buffer), "data[%d].iov_base", j);
                explain_buffer_efault(sb, buffer);
                return;
            }
        }
        return;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        return;

    case EINVAL:
        if (data_size < 0)
        {
            explain_buffer_einval_too_small(sb, "data_size", data_size);
            return;
        }

        /*
         * Linux writev(2) says
         * "POSIX.1-2001 allows an implementation to place a limit
         * on the number of items that can be passed in data.  An
         * implementation can advertise its limit by defining IOV_MAX
         * in <limits.h> or at run time via the return value from
         * sysconf(_SC_IOV_MAX).
         *
         * "On Linux, the limit advertised by these mechanisms is 1024,
         * which is the true kernel limit.  However, the glibc wrapper
         * functions do some extra work [to hide it]."
         */
        {
            long            iov_max;

            iov_max = -1;
#ifdef _SC_IOV_MAX
            iov_max = sysconf(_SC_IOV_MAX);
#endif
#if IOV_MAX > 0
            if (iov_max <= 0)
                iov_max = IOV_MAX;
#endif
            if (iov_max > 0 && data_size > iov_max)
            {
                explain_buffer_einval_too_large2(sb, "data_size", iov_max);
                return;
            }
        }

        /*
         * The total size must fit into a ssize_t return value.
         */
        {
            ssize_t max = (~(size_t)0) >> 1;
            long long total = 0;
            for (j = 0; j < data_size; ++j)
                total += data[j].iov_len;
            if (total > max)
            {
                explain_buffer_einval_too_large(sb, "data[*].iov_len");
                if (explain_option_dialect_specific())
                {
                    explain_string_buffer_puts(sb, " (");
                    explain_buffer_long_long(sb, total);
                    explain_string_buffer_puts(sb, " > ");
                    explain_buffer_ssize_t(sb, max);
                    explain_string_buffer_putc(sb, ')');
                }
                return;
            }
        }
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
        explain_buffer_enosys_vague(sb, syscall_name);
        return;

    default:
        break;
    }

    /*
     * We know the size fits, because we would otherwise get an EINVAL
     * error, and that would have been handled already.
     */
    data2 = (data_size > 0 ? data[0].iov_base : 0);
    data2_size = 0;
    for (j = 0; j < data_size; ++j)
        data2_size += data[j].iov_len;
    explain_buffer_errno_write_explanation
    (
        sb,
        errnum,
        syscall_name,
        fildes,
        data2,
        data2_size
    );
}


void
explain_buffer_errno_writev(explain_string_buffer_t *sb, int errnum, int fildes,
    const struct iovec *data, int data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_writev_system_call(&exp.system_call_sb, errnum, fildes,
        data, data_size);
    explain_buffer_errno_writev_explanation(&exp.explanation_sb, errnum,
        "writev", fildes, data, data_size);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
