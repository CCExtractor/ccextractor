/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/open.h>


void
explain_buffer_fildes(explain_string_buffer_t *sb, int fildes)
{
    if (fildes == AT_FDCWD)
    {
        char path[PATH_MAX + 1];
        explain_string_buffer_puts(sb, "AT_FDCWD");
        if (getcwd(path, sizeof(path)))
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, path);
        }
        else
        {
            explain_string_buffer_puts(sb, " \".\"");
        }
        return;
    }
    explain_string_buffer_printf(sb, "%d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
}


int
explain_parse_fildes_or_die(const char *text, const char *caption)
{
    /* First try, it could be a number */
    char *ep;
    long result = strtol(text, &ep, 0);
    if (text != ep && !*ep)
        return result;

    if (0 == strcmp(text, "AT_FDCWD"))
        return AT_FDCWD;

    /* Second try, it could be a specific stream */
    if (0 == strcmp(text, "stdin"))
        return fileno(stdin);
    if (0 == strcmp(text, "stdout"))
        return fileno(stdout);
    if (0 == strcmp(text, "stderr"))
        return fileno(stderr);

    /* Third try, it could be a pathname */
    return explain_open_or_die(text, O_RDONLY, 0);
    (void)caption;
}


/* vim: set ts=8 sw=4 et : */


/* vim: set ts=8 sw=4 et : */
