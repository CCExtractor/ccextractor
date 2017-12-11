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

#include <libexplain/is_efault.h>
#include <libexplain/output.h>
#include <libexplain/setenv.h>


#ifndef HAVE_SETENV

extern char **environ;


static char *
name_eq_value(const char *name, const char *value)
{
    size_t          name_size;
    size_t          value_size;
    char            *p;

    name_size = strlen(name);
    value_size = strlen(value);
    p = malloc(name_size + 1 + value_size + 1);
    if (!p)
        return NULL;
    memcpy(p, name, name_size);
    p[name_size] = '=';
    memcpy(p + name_size + 1, value, value_size);
    p[name_size + 1 + value_size] = '\0';
    return p;
}


static int
setenv(const char *name, const char *value, int overwrite)
{
    int             save_errno;
    size_t          name_size;
    char            **ep;
    size_t          environ_size;
    char            *p;
    char            **new_environ;

    save_errno = errno;
    if (!name)
    {
        errno = EINVAL;
        return -1;
    }
    if (explain_is_efault_string(name))
    {
        errno = EFAULT;
        return -1;
    }
    if (value && explain_is_efault_string(value))
    {
        errno = EFAULT;
        return -1;
    }
    if (!*name || strchr(name, '='))
    {
        errno = EINVAL;
        return -1;
    }
    name_size = strlen(name);
    ep = environ;
    if (!ep)
    {
        if (!value)
        {
            errno = save_errno;
            return 0;
        }
        environ = malloc(sizeof(char *) * 2); /* memory leak */
        if (!environ)
        {
            errno = ENOMEM;
            return -1;
        }
        environ[0] = 0;
        p = name_eq_value(name, value); /* memory leak */
        if (!p)
        {
            errno = ENOMEM;
            return -1;
        }
        environ[0] = p;
        environ[1] = NULL;
        errno = save_errno;
        return 0;
    }

    environ_size = 0;
    for (; *ep; ++ep)
    {
        if (!memcmp(*ep, name, name_size) && (*ep)[name_size] == '=')
        {
            if (!value)
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
                continue;
            }

            /* avoid doing anything, if possible */
            if (overwrite && 0 != strcmp(*ep + name_size + 1, value))
            {
                p = name_eq_value(name, value); /* memory leak */
                if (!p)
                {
                    errno = ENOMEM;
                    return -1;
                }
                *ep = p;
            }
            errno = save_errno;
            return 0;
        }
        ++environ_size;
    }
    if (!value)
    {
        errno = save_errno;
        return 0;
    }

    new_environ = malloc(sizeof(char *) * (environ_size + 2)); /* memory leak */
    if (!new_environ)
    {
        errno = ENOMEM;
        return -1;
    }
    memcpy(new_environ, environ, sizeof(char *) * environ_size);
    p = name_eq_value(name, value); /* memory leak */
    if (!p)
    {
        free(new_environ);
        errno = ENOMEM;
        return -1;
    }
    new_environ[environ_size] = p;
    new_environ[environ_size + 1] = NULL;
    environ = new_environ; /* memory leak */
    errno = save_errno;
    return 0;
}

#endif

int
explain_setenv_on_error(const char *name, const char *value, int overwrite)
{
    int             result;

    result = setenv(name, value, overwrite);
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_setenv(hold_errno, name,
            value, overwrite));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
