/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/fs.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/fildes_not_open_for_writing.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/iocontrol/blkdiscard.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/string_buffer.h>


#ifdef BLKDISCARD

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_uint64_array(sb, data, 2);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EBADF:
        if (explain_buffer_fildes_not_open_for_writing(sb, fildes, "fildes"))
            goto generic;
        break;

    case EFAULT:
        goto generic;

    case EINVAL:
        {
            const uint64_t  *u;
            uint64_t        start;
            uint64_t        len;

            u = data;
            start = u[0];
            len = u[1];
            if (start & 511)
                explain_buffer_einval_multiple(sb, "data[0]", 512);
            else if (len & 511)
                explain_buffer_einval_multiple(sb, "data[1]", 512);
            else
            {
                explain_buffer_einval_too_large(sb, "data[0] + data[1]");
#if defined(BLKGETSIZE64)
                {
                    uint64_t        dev_size_bytes;

                    if (ioctl(fildes, BLKGETSIZE64, &dev_size_bytes) >= 0)
                    {
                        explain_string_buffer_printf
                        (
                            sb,
                            " (%llu > %llu)",
                            (unsigned long long)start + len,
                            (unsigned long long)dev_size_bytes
                        );
                    }
                }
#elif defined(BLKGETSIZE)
                {
                    unsigned long   nblocks;

                    if (ioctl(fildes, BLKGETSIZE, &nblocks) >= 0)
                    {
                        explain_string_buffer_printf
                        (
                            sb,
                            " (%llu > %llu)",
                            (unsigned long long)start + len,
                            (unsigned long long)nblocks << 9
                        );
                    }
                }
#endif
            }
        }
        break;

    case EOPNOTSUPP:
        goto generic;

    case ENOMEM:
        goto generic;

    case EIO:
        goto generic;

    default:
        generic:
        explain_iocontrol_generic.print_explanation(p, sb, errnum, fildes,
            request, data);
        break;
    }
}


const explain_iocontrol_t explain_iocontrol_blkdiscard =
{
    "BLKDISCARD", /* name */
    BLKDISCARD, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(uint64_t[2]), /* data_size */
    "uint64_t[2]", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef BLKDISCARD */

const explain_iocontrol_t explain_iocontrol_blkdiscard =
{
    0, /* name */
    0, /* value */
    0, /* disambiguate */
    0, /* print_name */
    0, /* print_data */
    0, /* print_explanation */
    0, /* print_data_returned */
    0, /* data_size */
    0, /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#endif /* BLKDISCARD */


/* vim: set ts=8 sw=4 et : */
