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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/ide_task_request_t.h>
#include <libexplain/capability.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/hdio_drive_taskfile.h>
#include <libexplain/is_efault.h>

#ifdef HDIO_DRIVE_TASKFILE


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ide_task_request_t(sb, data);
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

    case EINVAL:
        {
            const ide_task_request_t *req_task = data;
            if (req_task->in_size > 65536)
            {
                explain_buffer_einval_too_large(sb, "data->in_size");
                explain_string_buffer_printf
                (
                    sb,
                    " (%lu > %lu)",
                    (unsigned long)req_task->in_size,
                    65536uL
                );
                break;
            }
            if (req_task->out_size > 65536)
            {
                explain_buffer_einval_too_large(sb, "data->out_size");
                explain_string_buffer_printf
                (
                    sb,
                    " (%lu > %lu)",
                    (unsigned long)req_task->out_size,
                    65536uL
                );
                break;
            }
        }
        goto generic;

    case EFAULT:
        if (explain_is_efault_pointer(data, sizeof(ide_task_request_t)))
        {
            explain_buffer_efault(sb, "data");
            break;
        }
        {
            const unsigned char *buf = data;
            const ide_task_request_t *req_task = data;
            if
            (
                req_task->out_size
            &&
                explain_is_efault_pointer
                (
                    buf + sizeof(ide_task_request_t),
                    req_task->out_size
                )
            )
            {
                explain_buffer_efault(sb, "data+outbuf");
                break;
            }
            if
            (
                req_task->in_size
            &&
                explain_is_efault_pointer
                (
                    buf + sizeof(ide_task_request_t),
                    req_task->in_size
                )
            )
            {
                explain_buffer_efault(sb, "data+inbuf");
                break;
            }
        }
        goto generic;

    case ENOMEM:
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


const explain_iocontrol_t explain_iocontrol_hdio_drive_taskfile =
{
    "HDIO_DRIVE_TASKFILE", /* name */
    HDIO_DRIVE_TASKFILE, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    0, /* print_data_returned */
    sizeof(ide_task_request_t), /* data_size */
    "ide_task_request_t *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef HDIO_DRIVE_TASKFILE */

const explain_iocontrol_t explain_iocontrol_hdio_drive_taskfile =
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

#endif /* HDIO_DRIVE_TASKFILE */


/* vim: set ts=8 sw=4 et : */
