/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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
#include <libexplain/ac/string.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/option.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>

#ifdef __linux__
#include <sys/syscall.h>
#include <linux/sysctl.h>
#endif


static long
get_maxfile(void)
{
    if (explain_option_dialect_specific())
    {
#ifdef __linux__
        /*
         * In the linux kernel, if get_empty_filp() returns NULL, the open
         * system call (and others) will return ENFILE.
         */
        struct __sysctl_args args;
        int32_t maxfile;
        size_t maxfile_size = sizeof(maxfile);
        int name[] = { CTL_FS, FS_MAXFILE };

        /*
         * The Linux sysctl(2) man page has this to say:
         *
         *     Glibc does not provide a wrapper for this system call; call it
         *     using syscall(2).
         *
         *     Or rather... don't call it: use of this system call has long
         *     been discouraged, and it is so unloved that it is likely
         *     to disappear in a future kernel version.  Remove it from your
         *     programs now; use the /proc/sys interface instead.
         *
         *     The object names vary between kernel versions, making this system
         *     call worthless for applications.
         *
         * Catch 22: you have to open a file to discover the limit of
         * open files AFTER you have hit the limit of open files.  Sigh.
         */
        memset(&args, 0, sizeof(struct __sysctl_args));
        args.name = name;
        args.nlen = SIZEOF(name);
        args.oldval = &maxfile;
        args.oldlenp = &maxfile_size;

        if (syscall(SYS__sysctl, &args) >= 0)
            return maxfile;
#endif
    }
    return -1;
}


void
explain_buffer_enfile(explain_string_buffer_t *sb)
{
    long            maxfile;

    /*
     * FIXME: say the limit -- except that there doesn't seem to be
     * a sysconf() name for this.  The _SC_OPEN_MAX name is for the
     * EMFILE error, which is different.
     */
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an ENFILE error.
         *
         * Note that it could be followed by the actual limit in
         * preentheses (if it can be determined) so it helps of the last
         * phrase in the message can sensably be followed by it.
         */
        i18n("the system limit on the total number of open files "
        "has been reached")
    );

    /*
     * Some systems provide a way to discover the actual limit.
     */
    maxfile = get_maxfile();
    if (maxfile > 0)
        explain_string_buffer_printf(sb, " (%ld)", maxfile);
}


/* vim: set ts=8 sw=4 et : */
