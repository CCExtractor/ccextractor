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
#include <libexplain/ac/limits.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/fileinfo.h>
#include <libexplain/lsof.h>


static int
proc_pid_exe(pid_t pid, char *data, size_t data_size)
{
#ifdef PROC_PID_CMDLINE
    {
        int             fd;
        char            path[100];

        snprintf(path, sizeof(path), "/proc/%ld/cmdline", (long)pid);
        fd = open(path, O_RDONLY);
        if (fd >= 0)
        {
            ssize_t         n;

            n = read(fd, data, data_size);
            close(fd);
            if (n > 0)
            {
                char            *cp;

                /*
                 * The data consists of each command line argument, and each is
                 * terminated by a NUL character, not a space.  Since we only
                 * care about the first one (argv[0], the command name) we just
                 * proceed as if there was only the one C string present.
                 */
                cp = memchr(data, '\0', n);
                if (cp)
                {
                    return 1;
                }
            }
        }
    }
#endif
#if defined(PROC_PID_PATH_A_OUT)
    if (data_size >= 2)
    {
        int             n;
        char            path[100];

        snprintf(path, sizeof(path), "/proc/%ld/path/a.out", (long)pid);
        n = readlink(path, data, data_size - 1);
        if (n > 0 && data[0] == '/')
        {
            data[n] = '\0';
            return 1;
        }
    }
#elif defined(PROC_PID_EXE)
    if (data_size >= 2)
    {
        int             n;
        char            path[100];

        snprintf(path, sizeof(path), "/proc/%ld/exe", (long)pid);
        n = readlink(path, data, data_size - 1);
        if (n > 0 && data[0] == '/')
        {
            data[n] = '\0';
            return 1;
        }
    }
#else
    (void)pid;
    (void)data;
    (void)data_size;
#endif

    /*
     * Sorry, can't help you.
     */
    return 0;
}


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
    if (context->fildes == LIBEXPLAIN_LSOF_FD_txt)
    {
        /*
         * Sometimes lsof(1) is less than helpful, and says "exe (readlink:
         * Permission denied)" which is effectively no answer at all.
         */
        if (0 == strstr(name, "exe (readlink:"))
        {
            adapter         *a;

            a = (adapter *)context;
            if (a->count == 0)
            {
                explain_strendcpy(a->data, name, a->data + a->data_size);
                a->count++;
            }
        }
    }
}


int
explain_fileinfo_pid_exe(pid_t pid, char *data, size_t data_size)
{
    /*
     * First, try to do it the easy way, through /proc
     */
    if (proc_pid_exe(pid, data, data_size))
    {
        return 1;
    }

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
        obj.count = 0;
        snprintf(options, sizeof(options), "-p %ld", (long)pid);
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
