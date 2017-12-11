/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/eoverflow.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/shmctl.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/shmctl_command.h>
#include <libexplain/buffer/shmid_ds.h>
#include <libexplain/buffer/shm_info.h>
#include <libexplain/buffer/shminfo.h>
#include <libexplain/explanation.h>
#include <libexplain/translate.h>


static int
wrap_shmctl(int shmid, int cmd, struct shmid_ds *data)
{
    if (shmctl(shmid, cmd, data) >= 0)
        return 0;
    if (errno == EACCES || errno == EPERM)
    {
        /*
         * Note that shmctl(IPC_STAT) requires read permission, so if we
         * get EACCES that means we know we have no read permission, but
         * we don't know anything else.  Fake it.
         */
        memset(data, 0, sizeof(*data));
        return 0;
    }
    return -1;
}


static void
explain_buffer_errno_shmctl_system_call(explain_string_buffer_t *sb, int errnum,
    int shmid, int command, struct shmid_ds *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "shmctl(shmid = ");
    explain_buffer_int(sb, shmid);
    explain_string_buffer_puts(sb, ", command = ");
    explain_buffer_shmctl_command(sb, command);
    explain_string_buffer_puts(sb, ", data = ");
    switch (command)
    {
#ifdef IPC_SET
    case IPC_SET:
        explain_buffer_shmid_ds(sb, data, 0);
        break;
#endif

#ifdef IPC_INFO
    case IPC_INFO:
#endif
#ifdef IPC_RMID
    case IPC_RMID:
#endif
#ifdef IPC_STAT
    case IPC_STAT:
#endif
#ifdef SHM_INFO
    case SHM_INFO:
#endif
#ifdef SHM_LOCK
    case SHM_LOCK:
#endif
#ifdef SHM_STAT
    case SHM_STAT:
#endif
#ifdef SHM_UNLOCK
    case SHM_UNLOCK:
#endif
    default:
        explain_buffer_pointer(sb, data);
        break;
    }
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_shmctl_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int shmid, int command, struct shmid_ds *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/shmctl.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (!data)
            explain_buffer_is_the_null_pointer(sb, "data");
        else
            explain_buffer_efault(sb, "data");
        break;

#ifdef EIDRM
    case EIDRM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This erro rmessage is used to describe the cause of an
             * EIDRM error reported by the shmctl(2) system call, in the case
             * where the shmid refers to a removed identifier.
             */
            i18n("shmid refers to a removed identifier")
        );
        break;
#endif

    case EINVAL:
        /*
         * 1. shmid is not a valid identifier; or
         * 2. command is not a valid command; or
         * 3. for a SHM_STAT operation, the index value specified in
         *    shmid referred to an array slot that is currently unused.
         */
        switch (command)
        {
#ifdef IPC_STAT
        case IPC_STAT:
#endif
#ifdef IPC_SET
        case IPC_SET:
#endif
#ifdef IPC_RMID
        case IPC_RMID:
#endif
#ifdef IPC_INFO
        case IPC_INFO:
#endif
#ifdef SHM_INFO
        case SHM_INFO:
#endif
#ifdef SHM_STAT
        case SHM_STAT:
#endif
#ifdef SHM_LOCK
        case SHM_LOCK:
#endif
#ifdef SHM_UNLOCK
        case SHM_UNLOCK:
#endif
            break;

        default:
            explain_buffer_einval_vague(sb, "command");
            break;
        }
        explain_buffer_einval_vague(sb, "data");
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case EOVERFLOW:
        explain_buffer_eoverflow(sb);
        break;

    case EACCES:
    case EPERM:
#ifdef SHM_LOCK
        if (command == SHM_LOCK || command == SHM_UNLOCK)
        {
            /*
             * "In kernels before 2.6.9, SHM_LOCK or SHM_UNLOCK was specified,
             * but the process was not privileged (Linux did not have the
             * CAP_IPC_LOCK capability).  Since Linux 2.6.9, this error can
             * also occur if the RLIMIT_MEMLOCK is 0 and the caller is not
             * privileged."
             */
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "the process does not have %s lock privileges",
                explain_translate_shared_memory_segment()
            );
            explain_buffer_dac_ipc_lock(sb);
        }
#endif

        {
            struct shmid_ds xyz;

            if (wrap_shmctl(shmid, IPC_STAT, &xyz) >= 0)
            {
                explain_buffer_eacces_shmat(sb, &xyz.shm_perm, 1);
                break;
            }
        }
        explain_buffer_eacces_shmat_vague(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_shmctl(explain_string_buffer_t *sb, int errnum, int shmid,
    int command, struct shmid_ds *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_shmctl_system_call(&exp.system_call_sb, errnum, shmid,
        command, data);
    explain_buffer_errno_shmctl_explanation(&exp.explanation_sb, errnum,
        "shmctl", shmid, command, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
