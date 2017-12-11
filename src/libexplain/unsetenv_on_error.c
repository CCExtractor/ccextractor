/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/unsetenv.h>
#include <libexplain/output.h>


#ifndef HAVE_UNSETENV

extern char     **environ;

/*
 * Maybe we could use putenv instead?
 *
 * The glibc implementation of putenv has enhanced semantics, in
 * that if there is no '=' in the argument to putenv, it does the
 * equivalent of unsetenv.  Of course, if we have glibc, then we already
 * have unsetenv.  The BSD implementations (and the Solarises, and
 * MacOSX) are not documentent to have the behaviour.  Thus, we ignore
 * HAVE_PUTENV because it probably can't help us here.
 */

static int
unsetenv(const char *name)
{
    char            **ep;
    size_t          name_size;

    if (!name || !*name || strchr(name, '='))
    {
        errno = EINVAL;
        return -1;
    }
    ep = environ;
    if (!ep)
        return 0;
    name_size = strlen(name);
    while (*ep)
    {
        if (!strncmp(*ep, name, name_size) && (*ep)[name_size] == '=')
        {
            char            **ep2;

            ep2 = ep;
            for (;;)
            {
                ep2[0] = ep2[1];
                if (!ep2[0])
                    break;
                ++ep2;
            }
            /* keep going, there could be more */
        }
        else
        {
            ++ep;
        }
    }
    return 0;
}

#endif

int
explain_unsetenv_on_error(const char *name)
{
    int             result;

    result = unsetenv(name);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_unsetenv(hold_errno, name));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
