/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/fileinfo.h>
#include <libexplain/is_efault.h>


void
explain_buffer_pid_t(explain_string_buffer_t *sb, pid_t pid)
{
    char            path[PATH_MAX + 1];

    explain_buffer_long(sb, pid);

    if (explain_fileinfo_pid_exe(pid, path, sizeof(path)))
    {
        const char      *cp;

        cp = strrchr(path, '/');
        if (cp)
            ++cp;
        else
            cp = path;
        explain_string_buffer_putc(sb, ' ');
        explain_string_buffer_puts_quoted(sb, cp);
    }
}


void
explain_buffer_pid_t_star(explain_string_buffer_t *sb, const pid_t *value)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_pid_t(sb, *value);
        explain_string_buffer_puts(sb, " }");
    }
}


void
explain_buffer_int_pid_star(explain_string_buffer_t *sb, const int *value)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_pid_t(sb, *value);
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
