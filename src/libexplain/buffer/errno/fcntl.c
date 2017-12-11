/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013, 2014 Peter Miller
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
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/check_fildes_range.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/errno/fcntl.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/flock.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/signal.h>
#include <libexplain/explanation.h>
#include <libexplain/fcntl.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


static const explain_parse_bits_table_t table[] =
{
    { "F_DUPFD",         F_DUPFD,         },
#ifdef F_DUPFD_CLOEXEC
    { "F_DUPFD_CLOEXEC", F_DUPFD_CLOEXEC, },
#endif
    { "F_GETFD",         F_GETFD,         },
    { "F_SETFD",         F_SETFD,         },
    { "F_GETFL",         F_GETFL,         },
    { "F_SETFL",         F_SETFL,         },
    { "F_GETLK",         F_GETLK,         },
    { "F_SETLK",         F_SETLK,         },
    { "F_SETLKW",        F_SETLKW,        },
    { "F_SETOWN",        F_SETOWN,        },
    { "F_GETOWN",        F_GETOWN,        },
#ifdef F_SETSIG
    { "F_SETSIG",        F_SETSIG,        },
#endif
#ifdef F_GETSIG
    { "F_GETSIG",        F_GETSIG,        },
#endif
#ifdef F_INPROGRESS
    { "F_INPROGRESS",    F_INPROGRESS,    },
#endif
#ifdef F_GETLEASE
    { "F_GETLEASE",      F_GETLEASE,      },
#endif
#ifdef F_SETLEASE
    { "F_SETLEASE",      F_SETLEASE,      },
#endif
#ifdef F_GETLK64
#if F_GETLK64 != F_GETLK
    { "F_GETLK64",       F_GETLK64,       },
    { "F_SETLK64",       F_SETLK64,       },
    { "F_SETLKW64",      F_SETLKW64,      },
#endif
#endif
#ifdef F_NOTIFY
    { "F_NOTIFY",        F_NOTIFY,        },
#endif
};


int
explain_fcntl_command_parse_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, sizeof(table), caption);
}


static void
explain_buffer_errno_fcntl_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, int command, long data)
{
    const explain_parse_bits_table_t *tp;

    explain_string_buffer_printf(sb, "fcntl(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", command = ");
    tp = explain_parse_bits_find_by_value(command, table, SIZEOF(table));
    if (tp)
        explain_string_buffer_puts(sb, tp->name);
    else
        explain_string_buffer_printf(sb, "%d", command);
    switch (command)
    {
    default:
        break;

    case F_DUPFD:
#ifdef F_DUPFD_CLOEXEC
    case F_DUPFD_CLOEXEC:
#endif
        explain_string_buffer_printf(sb, ", data = %ld", data);
        break;

    case F_SETFD:
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_open_flags(sb, data);
        break;

    case F_SETOWN:
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_pid_t(sb, data);
        break;

#ifdef F_SETSIG
    case F_SETSIG:
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_signal(sb, data);
        break;
#endif

#ifdef F_SETLEASE
    case F_SETLEASE:
        /* FIXME: decode lease flag */
        explain_string_buffer_printf(sb, ", data = %ld", data);
        break;
#endif

#ifdef F_NOTIFY
    case F_NOTIFY:
        /* FIXME: decode notify bits */
        explain_string_buffer_printf(sb, ", data = %ld", data);
        break;
#endif

    case F_SETFL:
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_open_flags(sb, data);
        break;

    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
        {
            const struct flock *p;

            p = (const struct flock *)data;
            explain_string_buffer_puts(sb, ", data = ");
            if (errnum == EFAULT)
                explain_buffer_pointer(sb, p);
            else
                explain_buffer_flock(sb, p);
        }
        break;

#ifdef F_GETLK64
#if F_GETLK64 != F_GETLK
    case F_GETLK64:
    case F_SETLK64:
    case F_SETLKW64:
        {
            const struct flock64 *p;

            p = (const struct flock64 *)data;
            explain_string_buffer_puts(sb, ", data = ");
            if (errnum == EFAULT)
                explain_buffer_pointer(sb, p);
            else
                explain_buffer_flock64(sb, p);
        }
        break;
#endif
#endif
    }
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_fcntl_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, int command, long data)
{
    switch (errnum)
    {
    case EACCES:
        /* FIXME: what other processes? */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the operation is prohibited by locks held by other processes"
        );
        break;

    case EAGAIN:
        /* FIXME: what other processes? */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the operation is prohibited by locks held by other "
            "processes; or, the operation is prohibited because the "
            "file has been memory-mapped by another process"
        );
        break;

    case EBADF:
        if (fcntl(fildes, F_GETFL) < 0)
        {
            explain_buffer_ebadf(sb, fildes, "fildes");
            break;
        }
        switch (command)
        {
        case F_SETLK:
        case F_SETLKW:
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the file descriptor open flags"
            );
            {
                /*
                 *  Insert the actual open mode here, if we can get it.
                 */
                int n = fcntl(fildes, F_GETFL);
                if (n >= 0)
                {
                    explain_string_buffer_puts(sb, " (");
                    explain_buffer_open_flags(sb, n);
                    explain_string_buffer_putc(sb, ')');
                }
            }
            explain_string_buffer_puts
            (
                sb,
                " do not match the type of lock requested"
            );
            break;

        default:
            goto generic;
        }
        break;

    case EDEADLK:
        /* FIXME: which other process? */
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "it was detected that the specified F_SETLKW "
            "command would cause a deadlock"
        );
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINTR:
        switch (command)
        {
        default:
            explain_buffer_eintr(sb, "command");
            break;

        case F_SETLK:
        case F_GETLK:
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "the command was interrupted by a signal "
                "before the lock was checked or acquired; most likely "
                "when locking a remote file (e.g. locking over NFS), "
                "but can sometimes happen locally"
            );
            break;
        }
        break;

    case EINVAL:
        switch (command)
        {
        case F_DUPFD:
#ifdef F_DUPFD_CLOEXEC
        case F_DUPFD_CLOEXEC:
#endif
            if (explain_buffer_check_fildes_range(sb, data, "data") < 0)
            {
                explain_buffer_einval_vague(sb, "data");
            }
            break;

#ifdef F_SETSIG
        case F_SETSIG:
            /* FIXME: devode signal number */
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "the %s argument is not an allowable signal number",
                "data"
            );
            break;
#endif

        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            {
                struct flock    *f;

                f = (struct flock *)data;
                if (!f)
                {
                    explain_buffer_is_the_null_pointer(sb, "data");
                    break;
                }
                if (explain_is_efault_pointer(f, sizeof(*f)))
                {
                    explain_buffer_efault(sb, "data");
                    break;
                }
                switch (f->l_type)
                {
                case F_RDLCK:
                case F_WRLCK:
                case F_UNLCK:
#ifdef F_UNLKSYS
                case F_UNLKSYS:
#endif
                    break;

                default:
                    explain_buffer_einval_vague(sb, "data->l_type");
                    goto done;
                }
                switch (f->l_whence)
                {
                case SEEK_SET:
                case SEEK_CUR:
                case SEEK_END:
                    break;

                default:
                    explain_buffer_einval_vague(sb, "data->l_whence");
                    goto done;
                }
                goto generic;
            }

        default:
            goto generic;
        }
        break;

    case EMFILE:
        switch (command)
        {
        case F_DUPFD:
#ifdef F_DUPFD_CLOEXEC
        case F_DUPFD_CLOEXEC:
#endif
            if (explain_buffer_check_fildes_range(sb, data, "data"))
                explain_buffer_emfile(sb);
            break;

        default:
            goto generic;
        }
        break;

    case ENOLCK:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "too many segment locks are open, or the lock table is full, "
            "or a remote locking protocol failed (e.g. locking over "
            "NFS)"
        );
        break;

    case EPERM:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "it was attempted to clear the O_APPEND flag on a file "
            "that has the append-only attribute set"
        );
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, "fcntl");
        break;
    }
    done:;
}


void
explain_buffer_errno_fcntl(explain_string_buffer_t *sb, int errnum,
    int fildes, int command, long data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fcntl_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        command,
        data
    );
    explain_buffer_errno_fcntl_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        command,
        data
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
