/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
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
#include <libexplain/ac/stdint.h>
#include <libexplain/ac/signal.h>
#include <libexplain/ac/sys/ptrace.h>
#include <libexplain/ac/sys/user.h>

#include <libexplain/buffer/ebusy.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/ptrace.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/ptrace_options.h>
#include <libexplain/buffer/ptrace_request.h>
#include <libexplain/buffer/ptrace_vm_entry.h>
#include <libexplain/buffer/signal.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/process_exists.h>


static void
explain_buffer_errno_ptrace_system_call(explain_string_buffer_t *sb, int errnum,
    int request, pid_t pid, void *addr, void *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "ptrace(request = ");
    explain_buffer_ptrace_request(sb, request);
    explain_string_buffer_puts(sb, ", pid = ");
    explain_buffer_pid_t(sb, pid);
    explain_string_buffer_puts(sb, ", addr = ");
    explain_buffer_pointer(sb, addr);
    explain_string_buffer_puts(sb, ", data = ");
    switch (request)
    {
#ifdef PT_CONTINUE
    case PT_CONTINUE:
#endif
#ifdef PT_SYSCALL
    case PT_SYSCALL:
#endif
#ifdef PT_STEP
    case PT_STEP:
#endif
#ifdef PT_SYSEMU
    case PT_SYSEMU:
#endif
#ifdef PT_SYSEMU_SINGLESTEP
    case PT_SYSEMU_SINGLESTEP:
#endif
#ifdef PT_DETACH
    case PT_DETACH:
#endif
        explain_buffer_signal(sb, (intptr_t)data);
        break;

#ifdef PT_SETOPTIONS
    case PT_SETOPTIONS:
#endif
#ifdef PT_OLDSETOPTIONS
    case PT_OLDSETOPTIONS:
#endif
        explain_buffer_ptrace_options(sb, (intptr_t)data);
        break;

#ifdef PT_WRITE_I
    case PT_WRITE_I:
#endif
#ifdef PT_WRITE_D
    case PT_WRITE_D:
#endif
#ifdef PT_WRITE_U
    case PT_WRITE_U:
#endif
        explain_buffer_long(sb, (intptr_t)data);
        break;

#ifdef PT_VM_ENTRY
    case PT_VM_ENTRY:
        explain_buffer_ptrace_vm_entry(sb, data);
        break;
#endif

    /* FIXME: FreeBSD's PT_IO uses struct ptrace_io_desc */

    default:
        explain_buffer_pointer(sb, data);
        break;
    }
    explain_string_buffer_putc(sb, ')');
}


static int
request_is_known(int request)
{
    switch (request)
    {
#ifdef PT_TRACE_ME
    case PT_TRACE_ME:
#endif
#ifdef PT_READ_I
    case PT_READ_I:
#endif
#ifdef PT_READ_D
    case PT_READ_D:
#endif
#ifdef PT_READ_U
    case PT_READ_U:
#endif
#ifdef PT_WRITE_I
    case PT_WRITE_I:
#endif
#ifdef PT_WRITE_D
    case PT_WRITE_D:
#endif
#ifdef PT_WRITE_U
    case PT_WRITE_U:
#endif
#ifdef PT_CONTINUE
    case PT_CONTINUE:
#endif
#ifdef PT_KILL
    case PT_KILL:
#endif
#ifdef PT_SINGLESTEP
    case PT_SINGLESTEP:
#endif
#ifdef PT_GETREGS
    case PT_GETREGS:
#endif
#ifdef PT_SETREGS
    case PT_SETREGS:
#endif
#ifdef PT_GETFPREGS
    case PT_GETFPREGS:
#endif
#ifdef PT_SETFPREGS
    case PT_SETFPREGS:
#endif
#ifdef PT_ATTACH
    case PT_ATTACH:
#endif
#ifdef PT_DETACH
    case PT_DETACH:
#endif
#ifdef PT_GETFPXREGS
    case PT_GETFPXREGS:
#endif
#ifdef PT_SETFPXREGS
    case PT_SETFPXREGS:
#endif
#ifdef PT_SYSCALL
    case PT_SYSCALL:
#endif
#ifdef PT_SETOPTIONS
    case PT_SETOPTIONS:
#endif
#ifdef PT_OLDSETOPTIONS
    case PT_OLDSETOPTIONS:
#endif
#ifdef PT_GETEVENTMSG
    case PT_GETEVENTMSG:
#endif
#ifdef PT_GETSIGINFO
    case PT_GETSIGINFO:
#endif
#ifdef PT_SETSIGINFO
    case PT_SETSIGINFO:
#endif
#ifdef PT_SYSEMU
    case PT_SYSEMU:
#endif
#ifdef PT_SYSEMU_SINGLESTEP
    case PT_SYSEMU_SINGLESTEP:
#endif
#ifdef PT_IO
    case PT_IO:
#endif
#ifdef PT_LWPINFO
    case PT_LWPINFO:
#endif
#ifdef PT_GETNUMLWPS
    case PT_GETNUMLWPS:
#endif
#ifdef PT_GETLWPLIST
    case PT_GETLWPLIST:
#endif
#ifdef PT_CLEARSTEP
    case PT_CLEARSTEP:
#endif
#ifdef PT_SETSTEP
    case PT_SETSTEP:
#endif
#ifdef PT_SUSPEND
    case PT_SUSPEND:
#endif
#ifdef PT_RESUME
    case PT_RESUME:
#endif
#ifdef PT_TO_SCE
    case PT_TO_SCE:
#endif
#ifdef PT_TO_SCX
    case PT_TO_SCX :
#endif
#ifdef PT_GETDBREGS
    case PT_GETDBREGS:
#endif
#ifdef PT_SETDBREGS
    case PT_SETDBREGS:
#endif
#ifdef PT_VM_TIMESTAMP
    case PT_VM_TIMESTAMP:
#endif
#ifdef PT_VM_ENTRY
    case PT_VM_ENTRY:
#endif
        return 1;

    default:
        break;
    }
    return 0;
}


static int
calculate_addr_size(int request)
{
    switch (request)
    {
#ifdef PT_READ_U
    case PT_READ_U:
#endif
#ifdef PT_WRITE_I
    case PT_WRITE_I:
#endif
#ifdef PT_WRITE_D
    case PT_WRITE_D:
#endif
#ifdef PT_WRITE_U
    case PT_WRITE_U:
#endif
        return sizeof(int);

    default:
        break;
    }
    return 0;
}


static int
calculate_data_size(int request)
{
    /*
     * The following structs are all defined in <sys/user.h> (on Linux,
     * anyway), so if your system doesn't have them, we can't give
     * useful answers.
     */
    switch (request)
    {
#ifdef SYS_PTRACE_USER_REGS_STRUCT
#ifdef PT_GETREGS
    case PT_GETREGS:
        return sizeof(struct user_regs_struct);
#endif
#ifdef PT_SETREGS
    case PT_SETREGS:
        return sizeof(struct user_regs_struct);
#endif
#endif /* SYS_PTRACE_USER_REGS_STRUCT */

#ifdef SYS_PTRACE_USER_FPREGS_STRUCT
#ifdef PT_GETFPREGS
    case PT_GETFPREGS:
        return sizeof(struct user_fpregs_struct);
#endif
#ifdef PT_SETFPREGS
    case PT_SETFPREGS:
        return sizeof(struct user_fpregs_struct);
#endif
#endif /* SYS_PTRACE_USER_FPREGS_STRUCT */

#ifdef SYS_PTRACE_USER_FPXREGS_STRUCT
#ifdef PT_GETFPXREGS
    case PT_GETFPXREGS:
        return sizeof(struct user_fpxregs_struct);
#endif
#ifdef PT_SETFPXREGS
    case PT_SETFPXREGS:
        return sizeof(struct user_fpxregs_struct);
#endif
#endif /* SYS_PTRACE_USER_FPREGS_STRUCT */

#ifdef PT_GETEVENTMSG
    case PT_GETEVENTMSG:
        return sizeof(unsigned long);
#endif
#ifdef PT_GETSIGINFO
    case PT_GETSIGINFO:
        return sizeof(siginfo_t);
#endif
#ifdef PT_SETSIGINFO
    case PT_SETSIGINFO:
        return sizeof(siginfo_t);
#endif

#ifdef PT_VM_ENTRY
    case PT_VM_ENTRY:
        /* FreeBSD, not Linux */
        return sizeof(struct ptrace_vm_entry);
#endif

    default:
        break;
    }
    return 0;
}

#if defined(PT_SETOPTIONS) || defined(PT_OLDSETOPTIONS)

static void
setting_an_invalid_option(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued to explain an EINVAL
         * error reported by the ptrace(2) system call, in the case
         * where an attempt was made to set an invalid option.
         */
        i18n("an attempt was made to set an invalid option")
    );
}

#endif

void
explain_buffer_errno_ptrace_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int request, pid_t pid, void *addr, void *data)
{
    size_t addr_size = calculate_addr_size(request);
    size_t data_size = calculate_data_size(request);

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/ptrace.html
     */
    switch (errnum)
    {
    case EBUSY:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain an EBUSY error
             * reported by the ptrace(2) system call, in the case where there
             * was an error with allocating or freeing a debug register.
             */
            i18n("there was an error with allocating or freeing a debug "
                "register")
        );
        break;

    case EFAULT:
        if (addr_size && explain_is_efault_pointer(addr, addr_size))
        {
            explain_buffer_efault(sb, "addr");
            break;
        }
        if (data_size && explain_is_efault_pointer(data, data_size))
        {
            explain_buffer_efault(sb, "data");
            break;
        }
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain an EFAULT error
             * reported by the ptrace(2) system call, in the cases where there
             * was an attempt to read from or write to an invalid area in the
             * parent's or child's memory, probably because the area wasn't
             * mapped or accessible.
             */
            i18n("there was an attempt to read from or write to an invalid "
                "area in the parent's or child's memory, probably because the "
                "area wasn't mapped or accessible")
        );
        /*
         * Unfortunately, under Linux, different variations of this
         * fault will return EIO or EFAULT more or less arbitrarily.
         */
        break;

    case EINVAL:
        if (!request_is_known(request))
        {
            explain_buffer_einval_vague(sb, "request");
            return;
        }
#ifdef PT_SETOPTIONS
        if (request == PT_SETOPTIONS)
        {
            setting_an_invalid_option(sb);
            break;
        }
#endif
#ifdef PT_OLDSETOPTIONS
        if (request == PT_OLDSETOPTIONS)
        {
            setting_an_invalid_option(sb);
            break;
        }
#endif
        goto generic;

    case EIO:
        if (!request_is_known(request))
        {
            explain_buffer_einval_vague(sb, "request");
            return;
        }
        if (addr_size && explain_is_efault_pointer(addr, addr_size))
        {
            explain_buffer_efault(sb, "addr");
            break;
        }
        if (data_size && explain_is_efault_pointer(data, data_size))
        {
            explain_buffer_efault(sb, "data");
            break;
        }
        switch (request)
        {
#ifdef PT_CONTINUE
        case PT_CONTINUE:
#endif
#ifdef PT_SYSCALL
        case PT_SYSCALL:
#endif
#ifdef PT_STEP
        case PT_STEP:
#endif
#ifdef PT_SYSEMU
        case PT_SYSEMU:
#endif
#ifdef PT_SYSEMU_SINGLESTEP
        case PT_SYSEMU_SINGLESTEP:
#endif
#ifdef PT_DETACH
        case PT_DETACH:
#endif
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This error message is issued to explain an EIO
                 * error reported by the ptrace(2) system call, in the case
                 * where an invalid signal was specified during a restart
                 * request.
                 */
                i18n("an invalid signal was specified during a restart request")
            );
            return;

        default:
            break;
        }
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain an EIO error
             * reported by the ptrace(2) system call, in the case where there
             * was a word-alignment violation.
             */
            i18n("there was a word-alignment violation")
        );
        break;

    case EPERM:
        if (!explain_process_exists(pid))
        {
            explain_buffer_eperm_kill(sb);
            break;
        }

        /*
         * Unprivileged processes cannot trace processes that are
         * running set-user-ID or set-group-ID programs.
         *
         * The process may already be being traced.
         * FIXME: can prctl tell us this?
         *
         * The process may be init(8), i.e. pid == 1
         */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain an EPERM error
             * reported by the ptrace(2) system call, in the case where the
             * specified process cannot be traced.
             */
            i18n("the specified process cannot be traced")
        );
        break;

    case ESRCH:
        if (pid <= 0 || kill(pid, 0) < 0)
        {
            explain_buffer_eperm_kill(sb);
            break;
        }

        /* FIXME: figure out which case applies */
        /* FIXME: i18n */
        explain_string_buffer_puts
        (
            sb,
            "the specified process is not currently being traced by the "
            "caller, or is not stopped (for requests that require that)"
        );
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_ptrace(explain_string_buffer_t *sb, int errnum, int
    request, pid_t pid, void *addr, void *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_ptrace_system_call(&exp.system_call_sb, errnum,
        request, pid, addr, data);
    explain_buffer_errno_ptrace_explanation(&exp.explanation_sb, errnum,
        "ptrace", request, pid, addr, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
