/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011-2013 Peter Miller
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
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/mman.h>
#include <libexplain/ac/sys/shm.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/shmat.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/must_be_multiple_of_page_size.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/shmflg.h>
#include <libexplain/explanation.h>
#include <libexplain/getpagesize.h>
#include <libexplain/translate.h>


static void
explain_buffer_errno_shmat_system_call(explain_string_buffer_t *sb, int errnum,
    int shmid, const void *shmaddr, int shmflg)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "shmat(shmid = ");
    explain_buffer_int(sb, shmid);
    explain_string_buffer_puts(sb, ", shmaddr = ");
    explain_buffer_pointer(sb, shmaddr);
    explain_string_buffer_puts(sb, ", shmflg = ");
    explain_buffer_shmflg(sb, shmflg);
    explain_string_buffer_putc(sb, ')');
}

#ifdef HAVE_MINCORE

static int
memory_already_mapped(const void *data, size_t data_size)
{
    int             page_size;
    unsigned char   *vec;

    page_size = explain_getpagesize();
    if (page_size < 0)
        return 0;
    vec = malloc((data_size + page_size - 1) / page_size);
    if (!vec)
        return 0;
    /*
     * The signed-ness of 'vec' differs by system.  There is a 50/50
     * chance you will get a compiler warning on the next line.
     */
    if (mincore((void *)data, data_size, vec) < 0)
    {
        free(vec);
        /*
         * Strictly speaking, only ENOMEM indicates that one or more
         * pages were not mapped.
         */
        return 0;
    }
    free(vec);
    return 1;
}

#endif

static int
shmid_exists(int shmid)
{
    struct shmid_ds shm;

    return
        (
            shmctl(shmid, IPC_STAT, &shm) >= 0
        ||
            errno == EACCES
        ||
            errno == EPERM
        );
}


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


void
explain_buffer_errno_shmat_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int shmid, const void *shmaddr, int shmflg)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/shmat.html
     */
    switch (errnum)
    {
    case EACCES:
    case EPERM:
        /*
         * The calling process does not have the required permissions for
         * the requested attach type, and does not have the CAP_IPC_OWNER
         * capability.
         */
        {
            struct shmid_ds shm;

            if (wrap_shmctl(shmid, IPC_STAT, &shm) >= 0)
            {
                int             read_only;

                read_only = !!(shmflg & SHM_RDONLY);
                if (explain_buffer_eacces_shmat(sb, &shm.shm_perm, read_only))
                    return;
            }
        }

        /*
         * We will have to settle for being vague.
         */
        explain_buffer_eacces_shmat_vague(sb);
        break;

    case EINVAL:
#ifdef SHM_REMAP
        if ((shmflg & SHM_REMAP) && !shmaddr)
        {
            explain_buffer_is_the_null_pointer(sb, "shmaddr");
            return;
        }
#endif
        /* use shmctl to find this out */
        if (!shmid_exists(shmid))
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This error message is used when explaining that a
                 * IPC object does not exist, when it should.
                 */
                i18n("the %s does not exist"),
                explain_translate_shared_memory_segment()
            );
            return;
        }
#ifdef HAVE_GETPAGESIZE
        if (shmaddr && !(shmflg & SHM_RND))
        {
            int             page_size;

            page_size = explain_getpagesize();
            if
            (
                page_size > 0
            &&
                ((size_t)shmaddr % (size_t)page_size) != 0
            )
            {
                explain_buffer_must_be_multiple_of_page_size(sb, "shmaddr");
                return;
            }
        }
#endif
#ifdef HAVE_MINCORE
        {
            struct shmid_ds shm;

            /*
             * The shmctl system call requires that the caller must have
             * read permission on the shared memory segment, so we may
             * not be able to be helpful if they aren't the owner (but
             * the we shouldn't get to here, that would be EACCES).
             */
            if
            (
                shmctl(shmid, IPC_STAT, &shm) >= 0
            &&
                memory_already_mapped(shmaddr, shm.shm_segsz)
            )
            {
                explain_string_buffer_printf
                (
                    sb,
                    /* FIXME: i18n */
                    "virtual memory has already been mapped at the "
                        "address indicated by the %s argument",
                    "shmaddr"
                );
                return;
            }
        }
#endif

        /* Nothing obvious... */
        goto generic;

    case ENOMEM: /* Linux */
    case EMFILE: /* POSIX */
        /*
         * See if the process's maximum number of mappings would have
         * been exceeded.
         */
        {
            struct shmid_ds shm;

            /*
             * The shmctl system call required that the caller must have
             * read permission on the shared memory segment.
             */
            if
            (
                shmctl(shmid, IPC_STAT, &shm) >= 0
            &&
                explain_buffer_enomem_rlimit_exceeded(sb, shm.shm_segsz)
            )
            {
                return;
            }
        }

        /*
         * Could not allocate memory
         * (a) for the descriptor, or
         * (b) for the page tables.
         */
        explain_buffer_enomem_kernel(sb);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_shmat(explain_string_buffer_t *sb, int errnum, int shmid,
    const void *shmaddr, int shmflg)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_shmat_system_call(&exp.system_call_sb, errnum, shmid,
        shmaddr, shmflg);
    explain_buffer_errno_shmat_explanation(&exp.explanation_sb, errnum, "shmat",
        shmid, shmaddr, shmflg);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
