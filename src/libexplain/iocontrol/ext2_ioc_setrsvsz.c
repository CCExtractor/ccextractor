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
#include <libexplain/ac/linux/ext2_fs.h>

#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/ext2_ioc_setrsvsz.h>

#ifdef EXT2_IOC_SETRSVSZ


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EFAULT:
        goto generic;

    case EACCES:
        explain_buffer_does_not_have_inode_modify_permission_fd
        (
            sb,
            fildes,
            "fildes"
        );
        break;

    case ENOTTY:
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


const explain_iocontrol_t explain_iocontrol_ext2_ioc_setrsvsz =
{
    "EXT2_IOC_SETRSVSZ", /* name */
    EXT2_IOC_SETRSVSZ, /* value */
    0, /* disambiguate */
    0, /* print_name */
    explain_iocontrol_generic_print_data_int_star, /* print_data */
    print_explanation,
    0, /* print_data_returned */
    sizeof(int), /* data_size */
    "int *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef EXT2_IOC_SETRSVSZ */

const explain_iocontrol_t explain_iocontrol_ext2_ioc_setrsvsz =
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

#endif /* EXT2_IOC_SETRSVSZ */


/* vim: set ts=8 sw=4 et : */
