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

#include <libexplain/ac/limits.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/fileinfo.h>
#include <libexplain/name_max.h>
#include <libexplain/program_name.h>


static char progname[NAME_MAX + 1];


static void
explain_program_name_set_real(const char *name)
{
    const char      *cp;

    if (!name)
        name = "";
    progname[0] = '\0';
    if (!name)
        return;

    cp = name;
    for (;;)
    {
        char            *pnp;

        if (*cp == '/')
        {
            /*
             * Weird but true: some Unix implementations allow trailing
             * slahses on command names.  Ugh.  Not POSIX conforming, either.
             */
            ++cp;
            continue;
        }
        if (*cp == '\0')
            break;

        /*
         * GNU Libtool makes intermediate binaries with "lt-" prefixes,
         * ignore them when we see them.
         */
        if (0 == memcmp(cp, "lt-", 3))
            cp += 3;

        pnp = progname;
        for (;;)
        {
            if (pnp < progname + sizeof(progname) - 1)
                *pnp++ = *cp;
            ++cp;
            if (*cp == '\0' || *cp == '/')
                break;
        }
        *pnp = '\0';
    }
}


const char *
explain_program_name_get(void)
{
    /*
     * Use the cached result, if possible.
     */
    if (progname[0])
        return progname;

    /*
     * See if /proc can help us.
     */
    {
        char            path[PATH_MAX + 1];

        if (explain_fileinfo_self_exe(path, sizeof(path)))
        {
            explain_program_name_set_real(path);
            if (progname[0])
                return progname;
        }
    }

    /*
     * bash(1) sets the "_" environment variable,
     * use that if available.
     */
    {
        const char      *path;

        path = getenv("_");
        if (path && *path)
        {
            explain_program_name_set_real(path);
            if (progname[0])
                return progname;
        }
    }

    /*
     * Sorry, can't help you.
     */
    return "";
}


void
explain_program_name_set(const char *name)
{
    explain_program_name_set_real(name);
    if (!progname[0])
        explain_program_name_assemble(0);
}


/* vim: set ts=8 sw=4 et : */
