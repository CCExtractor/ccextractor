/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/dirent.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/path_to_pid.h>
#include <libexplain/is_same_inode.h>
#include <libexplain/lsof.h>
#include <libexplain/string_buffer.h>


#if (defined(PROC_PID_PATH_CWD) || defined(PROC_PID_CWD)) && \
    (defined(PROC_PID_PATH_ROOT) || defined(PROC_PID_ROOT)) && \
    (defined(PROC_PID_PATH_A_OUT) || defined(PROC_PID_EXE)) && \
    (defined(PROC_PID_PATH_N) || defined(PROC_PID_FD_N))
#define TRY_PROC_FIRST 1
#endif


#ifdef TRY_PROC_FIRST

static int
snoop_symlink(const char *path, const struct stat *st1)
{
    char            link_content[PATH_MAX + 1];
    long            link_len;

    link_len = readlink(path, link_content, sizeof(link_content) - 1);
    if (link_len > 0)
    {
        struct stat     st4;

        link_content[link_len] = '\0';
        if
        (
            lstat(link_content, &st4) == 0
        &&
            explain_is_same_inode(st1, &st4)
        )
            return 1;
    }
    return 0;
}


static int
snoop_process(pid_t pid, const struct stat *st)
{
    char          path2[100];
    DIR           *dp2;

#if defined(PROC_PID_PATH_CWD)
    snprintf(path2, sizeof(path2), "/proc/%ld/path/cwd", (long)pid);
    if (snoop_symlink(path2, st))
        return 1;
#elif defined(PROC_PID_CWD)
    snprintf(path2, sizeof(path2), "/proc/%ld/cwd", (long)pid);
    if (snoop_symlink(path2, st))
        return 1;
#endif

#if defined(PROC_PID_PATH_A_OUT)
    snprintf(path2, sizeof(path2), "/proc/%d/path/a.out", pid);
    if (snoop_symlink(path2, st))
        return 1;
#elif defined(PROC_PID_EXE)
    snprintf(path2, sizeof(path2), "/proc/%d/exe", pid);
    if (snoop_symlink(path2, st))
        return 1;
#endif

#if defined(PROC_PID_PATH_ROOT)
    snprintf(path2, sizeof(path2), "/proc/%d/path/root", pid);
    if (snoop_symlink(path2, st))
        return 1;
#elif defined(PROC_PID_ROOT)
    snprintf(path2, sizeof(path2), "/proc/%d/root", pid);
    if (snoop_symlink(path2, st))
        return 1;
#endif

#if defined(PROC_PID_PATH_N)
    snprintf(path2, sizeof(path2), "/proc/%ld/path", (long)pid);
#elif defined(PROC_PID_FD_N)
    snprintf(path2, sizeof(path2), "/proc/%ld/fd", (long)pid);
#else
    #error oops
#endif
    dp2 = opendir(path2);
    if (!dp2)
        return 0;
    for (;;)
    {
        struct dirent   *dep2;
        char            *ep2;
        int             fd;
        char            path3[100];

        dep2 = readdir(dp2);
        if (!dep2)
            break;
        ep2 = 0;
        fd = strtol(dep2->d_name, &ep2, 10);
        if (ep2 == dep2->d_name || *ep2)
            continue;
        snprintf(path3, sizeof(path3), "%s/%d", path2, fd);
        if (snoop_symlink(path3, st))
        {
            closedir(dp2);
            return 1;
        }
    }
    closedir(dp2);
    return 0;
}


#else /* !TRY_PROC_FIRST */


typedef struct adapter adapter;
struct adapter
{
    explain_lsof_t inherited;
    explain_string_buffer_t *sb;
    const struct stat *st;
    int             count;
    const char      *caption;
};


static void
n_callback(explain_lsof_t *context, const char *name)
{
    adapter         *a;
    struct stat     st;

    a = (adapter *)context;
    if (lstat(name, &st) >= 0 && explain_is_same_inode(a->st, &st))
    {
        if (a->count == 0)
        {
            if (a->caption)
            {
                explain_string_buffer_putc(a->sb, ' ');
                explain_string_buffer_puts(a->sb, a->caption);
                explain_string_buffer_puts(a->sb, " is in use");
            }
            explain_string_buffer_puts(a->sb, " (pid");
        }
        else
            explain_string_buffer_putc(a->sb, ',');
        explain_string_buffer_printf(a->sb, " %d", context->pid);
        a->count++;
    }
}

#endif


static int
stat_to_pid(explain_string_buffer_t *sb, const struct stat *st)
{
#ifdef TRY_PROC_FIRST
    int             count;
    DIR             *dp;

    count = 0;
    dp = opendir("/proc");
    if (!dp)
        return -1;
    for (;;)
    {
        struct dirent *dep;
        char          *ep;
        int           pid;

        dep = readdir(dp);
        if (!dep)
            break;
        ep = 0;
        pid = strtol(dep->d_name, &ep, 10);
        if (ep == dep->d_name || *ep)
            continue;
        if (snoop_process(pid, st))
        {
            if (count == 0)
                explain_string_buffer_puts(sb, " (pid");
            else
                explain_string_buffer_putc(sb, ',');
            explain_string_buffer_printf(sb, " %d", pid);
            ++count;
        }
    }
    closedir(dp);
    if (count > 0)
        explain_string_buffer_putc(sb, ')');
    return count;
#else
    adapter         obj;

    obj.st = st;
    obj.inherited.n_callback = n_callback;
    obj.sb = sb;
    obj.count = 0;
    obj.caption = 0;
    explain_lsof("", &obj.inherited);
    if (obj.count > 0)
        explain_string_buffer_putc(sb, ')');
    return obj.count;
#endif
}


int
explain_buffer_path_to_pid(explain_string_buffer_t *sb, const char *path)
{
    struct stat     st;

    if (lstat(path, &st) < 0)
        return -1;
    return stat_to_pid(sb, &st);
}


int
explain_buffer_fildes_to_pid(explain_string_buffer_t *sb, int fildes)
{
    struct stat     st;

    if (fstat(fildes, &st) < 0)
        return -1;
    return stat_to_pid(sb, &st);
}


int
explain_buffer_path_users(explain_string_buffer_t *sb, const char *path,
    const char *caption)
{
#ifdef TRY_PROC_FIRST
    int             count;
    struct stat     st;
    DIR             *dp;

    if (lstat(path, &st) < 0)
        return -1;
    count = 0;
    dp = opendir("/proc");
    if (!dp)
        return -1;
    for (;;)
    {
        struct dirent *dep;
        char          *ep;
        int           pid;

        dep = readdir(dp);
        if (!dep)
            break;
        ep = 0;
        pid = strtol(dep->d_name, &ep, 10);
        if (ep == dep->d_name || *ep)
            continue;
        if (snoop_process(pid, &st))
        {
            if (count == 0)
            {
                explain_string_buffer_putc(sb, ' ');
                explain_string_buffer_puts(sb, caption);
                explain_string_buffer_puts(sb, " is in use");
                explain_string_buffer_puts(sb, " (pid");
            }
            else
                explain_string_buffer_putc(sb, ',');
            explain_string_buffer_printf(sb, " %d", pid);
            ++count;
        }
    }
    closedir(dp);
    if (count > 0)
        explain_string_buffer_putc(sb, ')');
    return count;
#else
    adapter         obj;
    struct stat     st;

    if (lstat(path, &st) < 0)
        return -1;
    obj.st = &st;
    obj.inherited.n_callback = n_callback;
    obj.sb = sb;
    obj.count = 0;
    obj.caption = caption;
    explain_lsof("", &obj.inherited);
    if (obj.count > 0)
        explain_string_buffer_putc(sb, ')');
    return obj.count;
#endif
}


/* vim: set ts=8 sw=4 et : */
