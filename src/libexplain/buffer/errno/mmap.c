/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/mman.h>
#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eagain.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/mmap.h>
#include <libexplain/buffer/etxtbsy.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/mmap_flags.h>
#include <libexplain/buffer/mmap_prot.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/must_be_multiple_of_page_size.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/wrong_file_type.h>
#include <libexplain/explanation.h>
#include <libexplain/getpagesize.h>
#include <libexplain/parse_bits.h>


static void
explain_buffer_errno_mmap_system_call(explain_string_buffer_t *sb, int errnum,
    void *data, size_t data_size, int prot, int flags, int fildes, off_t offset)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "mmap(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_puts(sb, ", prot = ");
    explain_buffer_mmap_prot(sb, prot);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_mmap_flags(sb, flags);
    explain_string_buffer_puts(sb, ", fildes = ");
#ifdef MAP_ANONYMOUS
    if (flags & MAP_ANONYMOUS)
        explain_buffer_int(sb, fildes);
    else
#endif
#ifdef MAP_ANON
    if (flags & MAP_ANON)
        explain_buffer_int(sb, fildes);
    else
#endif
        explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", offset = ");
    explain_buffer_off_t(sb, offset);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_mmap_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, void *data, size_t data_size, int prot, int flags,
    int fildes, off_t offset)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/mmap.html
     */
    switch (errnum)
    {
    case EACCES:
        {
            struct stat st;
            if (fstat(fildes, &st) == 0 && !S_ISREG(st.st_mode))
            {
                /*
                 * The file descriptor refers to a non-regular file.
                 */
                explain_buffer_wrong_file_type_st
                (
                    sb,
                    &st,
                    "fildes",
                    S_IFREG
                );
                break;
            }
        }

        /*
         * Or MAP_PRIVATE was requested, but fd is not open for reading.
         */
#ifdef MAP_PRIVATE
        if (flags & MAP_PRIVATE)
        {
            int omode = fcntl(fildes, F_GETFD, 0);
            if (omode >= 0 && (omode & O_ACCMODE) == O_WRONLY)
            {
                explain_buffer_ebadf_not_open_for_reading(sb, "fildes", omode);
                break;
            }
        }
#endif

#ifdef PROT_WRITE
        if (prot & PROT_WRITE)
        {
            int omode = fcntl(fildes, F_GETFD, 0);
            if (omode >= 0)
            {
#ifdef MAP_SHARED
                if (flags & MAP_SHARED)
                {
                    /*
                     * Or MAP_SHARED was requested and PROT_WRITE is set, but
                     * fildes is not open in read/write (O_RDWR) mode.
                     */
                    if ((omode & O_ACCMODE) != O_RDWR)
                    {
                        explain_buffer_gettext
                        (
                            sb,
                            /*
                             * xgettext: This message is used when an attempt is
                             * made to mmap shared access to a file descriptor
                             * that was not opened for both reading and writing.
                             * The actual open mode will be printed separately.
                             */
                            i18n("the file descriptor is not open for both "
                                "reading and writing")
                        );
                        explain_string_buffer_puts(sb, " (");
                        explain_buffer_open_flags(sb, omode);
                        explain_string_buffer_putc(sb, ')');
                        break;
                    }
                }
#endif

                /*
                 * Or PROT_WRITE is set, but the file is append-only.
                 */
                if (omode & O_APPEND)
                {
                    explain_buffer_gettext
                    (
                        sb,
                        /*
                         * xgettext: This message is used when an attempt is
                         * made to mmap write access to a file descriptor that
                         * is opened for append only.  The actual open mode will
                         * be printed separately.
                         */
                        i18n("the file descriptor is open for append")
                    );
                    explain_string_buffer_puts(sb, " (");
                    explain_buffer_open_flags(sb, omode);
                    explain_string_buffer_putc(sb, ')');
                    break;
                }
            }
        }
#endif
        goto generic;

    case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        /*
         * the file has been locked, or too much  memory  has  been  locked
         */
#ifdef MAP_LOCKED
        if (flags & MAP_LOCKED)
        {
            if (explain_buffer_enomem_rlimit_exceeded(sb, data_size))
                return;
        }
#endif
        /* FIXME: locked by whom? */
        explain_buffer_gettext
        (
            sb,
            /**
              * xgettext:  This message is used to explain an EAGAIN error
              * reported by the mmap(2) syatem call, in the case where the file
              * has been locked.
              */
            i18n("the file is locked")
        );
        break;

    case EBADF:
        /*
         * fildes is not a valid file descriptor (and MAP_ANONYMOUS was
         * not set).
         */
        explain_buffer_ebadf(sb, fildes, "fildes");
        /* assume this is an actual error, and do not check MAP_ANONYMOUS */
        break;

    case EINVAL:
        /*
         * We don't like addr, length, or offset (e.g., they are too large,
         * or not aligned on a page boundary).
         *
         * or (since Linux 2.6.12) length was 0
         */
#ifdef HAVE_GETPAGESIZE
        {
            int             page_size;

            page_size = explain_getpagesize();
            if (page_size > 0)
            {
                unsigned        mask;

                mask = (page_size - 1);
                if ((unsigned long)data & mask)
                {
                    explain_buffer_must_be_multiple_of_page_size(sb, "data");
                    break;
                }
                if ((unsigned long)data_size & mask)
                {
                    explain_buffer_must_be_multiple_of_page_size
                    (
                        sb,
                        "data_size"
                    );
                    break;
                }
                if ((unsigned long)offset & mask)
                {
                    explain_buffer_must_be_multiple_of_page_size(sb, "offset");
                    break;
                }
            }
        }
#endif
        if (data_size == 0)
        {
            explain_buffer_einval_too_small(sb, "data_size", data_size);
            break;
        }

        /*
         * or flags  contained neither MAP_PRIVATE or MAP_SHARED, or contained
         * both of these values.
         */
        switch (flags & (MAP_PRIVATE | MAP_SHARED))
        {
        case 0:
        case MAP_PRIVATE | MAP_SHARED:
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain in EINVAL error
                 * reported by mmap(2), in the case where the flags did not
                 * contain exactly one of MAP_PRIVATE or MAP_SHARED.
                 */
                i18n("you must specify exactly one of MAP_PRIVATE or "
                    "MAP_SHARED")
            );
            return;

        default:
            break;
        }
        goto generic;

    case ENFILE:
        /*
         * The system limit on the total number of open files has been reached.
         */
        explain_buffer_enfile(sb);
        break;

#ifdef ENOTSUP
    case ENOTSUP:
#endif
#ifdef EOPNOTSUP
    case EOPNOTSUP:
#endif
    case ENODEV:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain in ENODEV error
             * reported by mmap(2), in the case where the underlying
             * file system of the specified file does not support memory
             * mapping.
             */
            i18n("the underlying file system does not support memory mapping")
        );
        explain_buffer_mount_point_fd(sb, fildes);
        break;

    case ENOMEM:
        /*
         * No memory is available, or
         * the process's maximum number of mappings would have been exceeded.
         */
        if (explain_buffer_enomem_rlimit_exceeded(sb, data_size))
            return;
        explain_buffer_enomem_kernel(sb);
        break;

    case EPERM:
#ifdef PROT_EXEC
        if (prot & PROT_EXEC)
        {
            explain_buffer_gettext
            (
                sb,
                /**
                  * xgettext: This message is used to explain an EPERM error
                  * reported by the mmap(2) system call, in the case where the
                  * prot argument asks for PROT_EXEC but the mapped area belongs
                  * to a file on a file system that was mounted no-exec.
                  */
                i18n("the underlying file system does not permit execution")
            );
            explain_buffer_mount_point_fd(sb, fildes);
            break;
        }
#endif
        goto generic;

    case ETXTBSY:
#ifdef MAP_DENYWRITE
        if (flags & MAP_DENYWRITE)
        {
            int omode = fcntl(fildes, F_GETFD, 0);
            if (omode >= 0)
            {
                switch (omode & O_ACCMODE)
                {
                case O_WRONLY:
                case O_RDWR:
                    explain_buffer_gettext
                    (
                        sb,
                        /**
                          * xgettext: This message is used to explain an ETXTBSY
                          * error reported by a mmap(2) system call, in the case
                          * where MAP_DENYWRITE was set but the object specified
                          * by the file descriptor is open for writing.
                          * The file's open mode is printed separately.
                          */
                        i18n("the mapping flag MAP_DENYWRITE is incompatible "
                            "with the open mode of the file descriptor")
                    );
                    explain_string_buffer_puts(sb, " (");
                    explain_buffer_open_flags(sb, omode);
                    explain_string_buffer_putc(sb, ')');
                    return;

                default:
                    break;
                }
            }
        }
#endif
        goto generic;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_mmap(explain_string_buffer_t *sb, int errnum, void *data,
    size_t data_size, int prot, int flags, int fildes, off_t offset)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_mmap_system_call(&exp.system_call_sb, errnum, data,
        data_size, prot, flags, fildes, offset);
    explain_buffer_errno_mmap_explanation(&exp.explanation_sb, errnum, "mmap",
        data, data_size, prot, flags, fildes, offset);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
