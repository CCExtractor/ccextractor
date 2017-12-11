/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/lsof.h>
#include <libexplain/option.h>


void
explain_lsof(const char *lsof_options, explain_lsof_t *context)
{
    FILE            *fp;
    char            command[200];

    if (!lsof_options)
        lsof_options = "";
    snprintf
    (
        command,
        sizeof(command),
        "lsof -Ffnp0 %s %s %s",
        (lsof_options[0] ? "-a" : ""),
        lsof_options,
        (explain_option_debug() >= 2 ? "" : "2> /dev/null")
    );
    fp = popen(command, "r");
    if (!fp)
        return;
    context->pid = 0;
    context->fildes = -1;
    for (;;)
    {
        char            line[PATH_MAX + 2];
        char            *lp;

        lp = line;
        for (;;)
        {
            int             c;

            c = getc(fp);
            if (c == EOF)
            {
                *lp++ = '\0';
                break;
            }
            if (c == '\0')
            {
                *lp++ = '\0';
                /*
                 * The lsof(1) man page says that -F0 NUL terminates the
                 * lines, but it actually terminates them with "\0\n"
                 * instead, except when it uses just "\0".  Sheesh.
                 */
                c = getc(fp);
                if (c != EOF && c != '\n')
                    ungetc(c, fp);
                break;
            }
            if (lp < line + sizeof(line) - 1)
                *lp++ = c;
        }
        switch (line[0])
        {
        case '\0':
            pclose(fp);
            return;

        case 'p':
            context->pid = atoi(line + 1);
            context->fildes = -1;
            break;

        case 'f':
            context->fildes = atoi(line + 1);
            if (context->fildes == 0)
            {
                if (isalpha((unsigned char)line[1]))
                    context->fildes = -line[1];
            }
            break;

        case 'n':
            if (context->n_callback)
                (*context->n_callback)(context, line + 1);
            break;

        default:
            /* ignore everything else */
            break;
        }
    }
}


/* vim: set ts=8 sw=4 et : */
