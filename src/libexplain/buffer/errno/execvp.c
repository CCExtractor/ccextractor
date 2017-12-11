/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/execve.h>
#include <libexplain/buffer/errno/execvp.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/have_permission.h>
#include <libexplain/name_max.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


static size_t
count(char *const *p)
{
    size_t          result;

    result = 0;
    while (*p)
    {
        ++result;
        ++p;
    }
    return result;
}


static void
explain_buffer_errno_execvp_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, char *const *argv)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "execvp(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", argv = ");
    if (explain_is_efault_pointer(argv, sizeof(argv[0])))
        explain_buffer_pointer(sb, argv);
    else
    {
        size_t          argc;
        size_t          n;
        size_t          argsize;

        /*
         * produce output similar to execve
         */
        argc = count(argv);
        explain_string_buffer_putc(sb, '[');
        argsize = 0;
        for (n = 0; n < argc && argsize < 1000; ++n)
        {
            const char      *s;

            if (n)
                explain_string_buffer_puts(sb, ", ");
            s = argv[n];
            if (explain_is_efault_path(s))
            {
                explain_buffer_pointer(sb, s);
                argsize += 8;
            }
            else
            {
                explain_string_buffer_puts_quoted(sb, s);
                argsize += strlen(s);
            }
        }
        if (n < argc)
        {
            explain_string_buffer_printf
            (
                sb,
                /* FIXME: i18n */
                "... plus another %d command line arguments",
                (int)(count(argv) - n)
            );
        }
        explain_string_buffer_putc(sb, ']');
    }
    explain_string_buffer_putc(sb, ')');
}


static int
can_execute(const char *pathname)
{
    struct stat     st;
    explain_have_identity_t hid;

    if (stat(pathname, &st) < 0)
        return 0;
    explain_have_identity_init(&hid);
    errno = EACCES;
    return explain_have_execute_permission(&st, &hid);
}


void
explain_buffer_errno_execvp_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    char *const *argv)
{
    const char      *p;
    const char      *path;
    char            dpath[100];
    size_t          path_len;
    size_t          pathname_len;
    size_t          full_pathname_len;
    char            *full_pathname;
    char            *start_of_name;

    if (explain_is_efault_path(pathname))
    {
        explain_buffer_efault(sb, "pathname");
        return;
    }

    path = getenv("PATH");
    if (!path)
    {
        if (!confstr(_CS_PATH, dpath, sizeof(dpath)))
            explain_strendcpy(dpath, ".:/bin:/usr/bin", ENDOF(dpath));
        path = dpath;
    }
    path_len = strlen(path);

    if (errnum == ENOENT)
    {
        char qcmd[NAME_MAX];
        explain_string_buffer_t qcmd_sb;
        char qpath[1000]; /* deliberately short */
        explain_string_buffer_t qpath_sb;

        explain_string_buffer_init(&qcmd_sb, qcmd, sizeof(qcmd));
        explain_buffer_caption_name_type(&qcmd_sb, 0, pathname, S_IFREG);

        explain_string_buffer_init(&qpath_sb, qpath, sizeof(qpath));

        p = path;
        for (;;)
        {
            const char      *begin;

            begin = p;
            p = strchr(begin, ':');
            if (!p)
                p = path + strlen(path);

            if (qpath_sb.position != 0)
                explain_string_buffer_puts(&qpath_sb, ", ");
            if (p == begin)
            {
                explain_string_buffer_puts_quoted(&qpath_sb, ".");
            }
            else
            {
                size_t          len;

                len = p - begin;
                if
                (
                    qpath_sb.position != 0
                &&
                    qpath_sb.position + len + 5 > qpath_sb.maximum
                )
                {
                    explain_string_buffer_puts(&qpath_sb, "...");
                    break;
                }
                explain_string_buffer_puts_quoted_n(&qpath_sb, begin, len);
            }

            if (*p == '\0')
                break;
            ++p;
        }

        explain_string_buffer_printf
        (
            sb,
            /*
             * xgettext: This message is used to explain an ENOENT error
             * returned by the execvp(3) system call.
             *
             * %1$s => the name and file type of the command, already quoted.
             *         e.g. "\"bogus\" regular file"
             * %2$s => the command search PATH, already quoted.
             */
            i18n("there is no %s in any of the command search PATH "
                "directories (%s)"),
            qcmd,
            qpath
        );
        return;
    }

    if (!*pathname || strchr(pathname, '/'))
    {
        goto give_default_explanation;
    }

    /*
     * Now we simulate execvp, based on the logic in the code from
     * glibc, and heavily modified.
     */
    pathname_len = strlen(pathname);
    full_pathname_len = path_len + pathname_len + 2;
    full_pathname = malloc(full_pathname_len);
    if (!full_pathname)
    {
        goto give_default_explanation;
    }

    /*
     * Copy the pathname at the top,
     * and add the slash.
     */
    start_of_name = full_pathname + path_len + 1;
    memcpy(start_of_name, pathname, pathname_len + 1);
    *--start_of_name = '/';

    p = path;
    for (;;)
    {
        const char      *begp;
        char            *command_path;

        begp = p;
        p = strchr(begp, ':');
        if (!p)
            p = begp + strlen(begp);

        if (p == begp)
        {
            /*
             * Two adjacent colons, or a colon at the beginning or the
             * end of PATH, means to search the current directory.
             */
            command_path = start_of_name + 1;
        }
        else
        {
            size_t          part_len;

            part_len = p - begp;
            command_path = start_of_name - part_len;
            memcpy(command_path, begp, part_len);
        }

        /*
         * The real execvp tries to execute this name..  If it works,
         * execve will not return.
         *     execve(command_path, argv, environ);
         * We are simulating execvp, so if the file is executable, we
         * failed to preproduce the problem.
         */
        if (can_execute(command_path))
        {
            /* unable to reproduce the problem */
            free(full_pathname);
            return;
        }
        if (errno == errnum)
        {
            found_it:
            explain_buffer_errno_execve_explanation
            (
                sb,
                errnum,
                syscall_name,
                command_path,
                argv,
                environ
            );
            free(full_pathname);
            return;
        }
        /*
         * Here we are switching on the errno returned by can_execute,
         * to see what went wrong with this attempt (NOT errnum).
         */
        switch (errno)
        {
        case EACCES:
            goto found_it;

        case ENOENT:
        case ESTALE:
        case ENOTDIR:
            /*
             * Those errors indicate the file is missing or not
             * executable by us, in which case we want to just try the
             * next path directory.
             */
            break;

        case ENODEV:
        case ETIMEDOUT:
            /*
             * Some strange filesystems like AFS return even stranger
             * error numbers.  They cannot reasonably mean anything else
             * so ignore those, too.
             */
            break;

        default:
            goto found_it;
        }
        if (*p == '\0')
            break;
        ++p;
    }
    free(full_pathname);

    /*
     * Nothing happened, give the default explanation.
     */
    give_default_explanation:
    explain_buffer_errno_execve_explanation
    (
        sb,
        errnum,
        syscall_name,
        pathname,
        argv,
        environ
    );
}


void
explain_buffer_errno_execvp(explain_string_buffer_t *sb, int errnum,
    const char *pathname, char *const *argv)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_execvp_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        argv
    );
    explain_buffer_errno_execvp_explanation
    (
        &exp.explanation_sb,
        errnum,
        "execvp",
        pathname,
        argv
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
