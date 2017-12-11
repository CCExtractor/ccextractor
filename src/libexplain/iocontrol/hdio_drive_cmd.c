/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/hdreg.h>
#include <libexplain/ac/linux/msdos_fs.h> /* for SECTOR_SIZE */

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/hdio_drive_cmd.h>

#include <libexplain/buffer/int.h>

#ifdef HDIO_DRIVE_CMD


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_char_data(sb, data, 4);
}


#define min(a, b) ((a) < (b) ? (a) : (b))


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    size_t          len;
    const unsigned char *args;

    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    args = data;
    /*
     * args[0] = status
     * args[1] = error
     * args[2] = sector count (output)
     * args[3] = sector count (input)
     */
    len = 4 + SECTOR_SIZE * min(args[2], args[3]);
    explain_buffer_char_data(sb, data, len);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EACCES:
        /* you need both CAP_SYS_ADMIN and CAP_SYS_RAWIO */
        if (!explain_capability_sys_admin())
        {
            explain_buffer_dac_sys_admin(sb);
            break;
        }
        if (!explain_capability_sys_rawio())
        {
            explain_buffer_dac_sys_rawio(sb);
            break;
        }
        goto generic;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            break;
        }
        goto generic;

    case ENOMEM:
    case EIO:
        goto generic;

    default:
        generic:
        explain_iocontrol_generic_print_explanation
        (
            p,
            sb,
            errnum,
            fildes,
            request,
            data
        );
        break;
    }
}


/*
 * The data_size is tricky.  On entry, it is always exactly 4.
 * The returned data, however, can be up to 4+255*SECTOR_SIZE.
 */
const explain_iocontrol_t explain_iocontrol_hdio_drive_cmd =
{
    "HDIO_DRIVE_CMD", /* name */
    HDIO_DRIVE_CMD, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
    sizeof(unsigned char[4]), /* data_size */
    "unsigned char[4]", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef HDIO_DRIVE_CMD */

const explain_iocontrol_t explain_iocontrol_hdio_drive_cmd =
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

#endif /* HDIO_DRIVE_CMD */


/* vim: set ts=8 sw=4 et : */
