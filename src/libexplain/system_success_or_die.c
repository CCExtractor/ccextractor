/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/wait.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/errno/access.h>
#include <libexplain/buffer/errno/fork.h>
#include <libexplain/buffer/errno/system.h>
#include <libexplain/buffer/errno/wait.h>
#include <libexplain/buffer/wait_status.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/string_buffer.h>
#include <libexplain/system.h>
#include <libexplain/output.h>


static int
extract_command(const char *src, char *dst, size_t dst_size)
{
    char *dst_end = dst + dst_size - 1;
    char *dp;

    for (;;)
    {
        if (!*src)
            break;
        if (isspace((unsigned char)*src))
        {
            ++src;
            continue;
        }
        dp = dst;
        for (;;)
        {
            if (dp < dst_end)
                *dp++ = *src;
            ++src;
            if (!*src)
                break;
            if (isspace((unsigned char)*src) || strchr("<>^|", *src))
            {
                ++src;
                break;
            }
        }
        *dp = '\0';

        /*
         * If there is an equals sign, it is probably an environment
         * variable setting before the command word.
         */
        if (strchr(dst, '='))
            continue;

        /*
         * Probably found the command, so long as it contains no shell
         * special characters.
         */
        return !strpbrk(dst, "!\"#$&'()*:;<>?[\\]^`{|}");
    }
    return 0;
}


static int
command_on_path(const char *cmd)
{
    char            combo[PATH_MAX * 2 + 3];
    explain_string_buffer_t combo_sb;
    const char      *path;
    const char      *start;

    path = getenv("PATH");
    if (!path)
        path = ".";
    for (;;)
    {
        start = path;
        for (;;)
        {
            if (*path == '\0')
                break;
            if (*path == ':')
                break;
            ++path;
        }
        explain_string_buffer_init(&combo_sb, combo, sizeof(combo));
        if (start == path)
            explain_string_buffer_putc(&combo_sb, '.');
        else
            explain_string_buffer_write(&combo_sb, start, path - start);
        explain_string_buffer_path_join(&combo_sb, cmd);
        if (access(combo, X_OK) >= 0)
            return 1;
        if (*path == '\0')
            return 0;
        assert(*path == ':');
        ++path;
    }
}


int
explain_system_success(const char *command)
{
    int             status;

    status = system(command);
    if (status < 0)
    {
        explain_output_error("%s", explain_system(command));
    }
    else if (status != 0)
    {
        explain_string_buffer_t sb;
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        explain_buffer_errno_system(&sb, 0, command);

        if (!command)
            command = "/bin/sh";

        /* FIXME: i18n */
        explain_string_buffer_puts(&sb, ", but ");
        explain_buffer_wait_status(&sb, status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        {
            char            cmd[PATH_MAX + 1];

            /*
             * It is possible that execl(2) used by system(3) failed,
             * and this could have caused the 127 exit status.  On the
             * other hand, the command itself could return this.
             */
            if (extract_command(command, cmd, sizeof(cmd)))
            {
                if (strchr(cmd, '/'))
                {
                    if (access(cmd, X_OK) < 0)
                    {
                        explain_string_buffer_puts(&sb, ", ");
                        explain_buffer_errno_access_explanation
                        (
                            &sb,
                            errno,
                            "system",
                            cmd,
                            X_OK
                        );
                    }
                }
                else if (!command_on_path(cmd))
                {
                    explain_string_buffer_puts(&sb, ", ");
                    explain_string_buffer_puts_quoted(&sb, cmd);
                    explain_string_buffer_puts
                    (
                        &sb,
                        " command not found on $PATH"
                    );
                }
            }
        }
        explain_output_error("%s", explain_common_message_buffer);
    }
    return status;
}


void
explain_system_success_or_die(const char *command)
{
    if (explain_system_success(command))
        explain_output_exit_failure();
}


/* vim: set ts=8 sw=4 et : */
