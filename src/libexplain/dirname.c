/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/dirname.h>


void
explain_dirname(char *dir, const char *path, size_t dir_size)
{
    const char      *slash;
    size_t          len;
    char            current_directory[PATH_MAX + 1];

    if (dir_size == 0)
        return;
    --dir_size;

    if (0 == strcmp(path, "/"))
    {
        explain_strendcpy(dir, "/", dir + dir_size);
        return;
    }
    if (0 == strcmp(path, "."))
    {
        if (!getcwd(current_directory, sizeof(current_directory)))
        {
            explain_strendcpy(dir, "..", dir + dir_size);
            return;
        }
        path = current_directory;
    }

    slash = strrchr(path, '/');
    if (!slash)
    {
        path = "./";
        slash = path + 1;
    }
    if (slash == path)
    {
        explain_strendcpy(dir, "/", dir + dir_size);
        return;
    }
    len = slash - path;
    if (len > dir_size)
        len = dir_size;
    memcpy(dir, path, len);
    dir[len] = '\0';
}


void
explain_basename(char *filename, const char *path, size_t filename_size)
{
    char *          filename_end;
    const char      *slash;

    filename_end = filename + filename_size;
    slash = strrchr(path, '/');
    if (slash)
    {
        ++slash;
        if (*slash)
            explain_strendcpy(filename, slash, filename_end);
        else
            explain_strendcpy(filename, ".", filename_end);
    }
    else
    {
        explain_strendcpy(filename, path, filename_end);
    }
}


/* vim: set ts=8 sw=4 et : */
