/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/fileinfo.h>
#include <libexplain/lsof.h>


typedef struct adapter adapter;
struct adapter
{
    explain_lsof_t  inherited;
    char            *data;
    size_t          data_size;
    int             found;
};


static void
n_callback(explain_lsof_t *context, const char *name)
{
    /*
     * Sometimes lsof(1) is less than helpful, and says "n (readlink:
     * Permission denied)" which is effectively no answer at all.
     *
     * There is a very small chance of discarding a valid result.
     * Get fussier if it proves to be an actual problem.
     */
    if (!strstr(name, " (readlink: "))
    {
        adapter         *a;

        a = (adapter *)context;
        a->found = 1;
        explain_strendcpy(a->data, name, a->data + a->data_size);
    }
}


int
explain_fileinfo_pid_fd_n(pid_t pid, int fildes, char *data, size_t data_size)
{
    if (fildes == AT_FDCWD)
    {
        return explain_fileinfo_self_cwd(data, data_size);
    }
    if (data_size < 2)
        return 0;
#if defined(PROC_PID_PATH_N)
    {
        int             n;
        char            path[PATH_MAX + 1];

        /* Solaris */
        snprintf(path, sizeof(path), "/proc/%ld/path/%d", (long)pid, fildes);
        n = readlink(path, data, data_size - 1);
        if (n > 0 && data[0] == '/')
        {
            data[n] = '\0';
            return 1;
        }
    }
#elif defined(PROC_PID_FD_N)
    {
        int             n;
        char            path[PATH_MAX + 1];

        /* Linux (The Solaris files at the same path aren't useful). */
        snprintf(path, sizeof(path), "/proc/%ld/fd/%d", (long)pid, fildes);
        n = readlink(path, data, data_size - 1);
        if (n > 0 && data[0] == '/')
        {
            data[n] = '\0';
            return 1;
        }
    }
#endif

    /*
     * It is possible that /proc didn't work.  Maybe ./configure
     * was fooled, or it could be that /proc worked in the build
     * environment, but it isn't available or doesn't work in the
     * runtime environment (e.g. chroot jails).
     *
     * Try lsof(1) instead.
     */
    {
        adapter         obj;
        char            options[40];

        obj.inherited.n_callback = n_callback;
        obj.data = data;
        obj.data_size = data_size;
        obj.found = 0;
        snprintf(options, sizeof(options), "-p %ld -d %d", (long)pid, fildes);
        explain_lsof(options, &obj.inherited);
        if (obj.found)
            return 1;
    }

    /*
     * Sorry, can't help you.
     */
    return 0;
}


/* vim: set ts=8 sw=4 et : */
