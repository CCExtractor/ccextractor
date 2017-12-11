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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/fileinfo.h>
#include <libexplain/lsof.h>


#if defined(PROC_PID_PATH_CWD) || defined(PROC_PID_CWD)

static int
proc_pid_cwd(pid_t pid, char *data, size_t data_size)
{
    int             n;
    char            path[100];

    if (data_size < 2)
        return 0;
#if defined(PROC_PID_PATH_CWD)
    snprintf(path, sizeof(path), "/proc/%ld/path/cwd", (long)pid);
#else
    snprintf(path, sizeof(path), "/proc/%ld/cwd", (long)pid);
#endif
    n = readlink(path, data, data_size - 1);
    if (n <= 0)
        return 0;
    data[n] = '\0';
    return (data[0] == '/');
}

#endif


typedef struct adapter adapter;
struct adapter
{
    explain_lsof_t  inherited;
    char            *data;
    size_t          data_size;
    int             count;
};


static void
n_callback(explain_lsof_t *context, const char *name)
{
    /*
     * Sometimes lsof(1) is less than helpful, and says "cwd (readlink:
     * Permission denied)" which is effectively no answer at all.
     */
    if (0 != memcmp(name, "cwd (readlink:", 14))
    {
        adapter         *a;

        a = (adapter *)context;
        explain_strendcpy(a->data, name, a->data + a->data_size);
        a->count++;
    }
}


int
explain_fileinfo_pid_cwd(pid_t pid, char *data, size_t data_size)
{
    /*
     * First, we try to do it the easy way.
     */
#if defined(PROC_PID_PATH_CWD) || defined(PROC_PID_CWD)
    if (proc_pid_cwd(pid, data, data_size))
        return 1;
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

        snprintf(options, sizeof(options), "-p%ld -dcwd", (long)pid);
        obj.inherited.n_callback = n_callback;
        obj.data = data;
        obj.data_size = data_size;
        obj.count = 0;
        explain_lsof(options, &obj.inherited);
        if (obj.count > 0)
            return 1;
    }

    /*
     * Sorry, can't help you.
     */
    return 0;
}


/* vim: set ts=8 sw=4 et : */
