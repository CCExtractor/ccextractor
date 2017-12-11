/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/dirent.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris  */

#include <libexplain/buffer/device_name.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


/**
  * The dev_stat_rec function is used to walk the /dev directory
  * tree, looking for a specific device.
  *
  * @param dev_path
  *     The is the path recursively constructed as we descend down the
  *     directory tree looking for devices.
  * @param dev
  *     The device beeing searched for.
  * @param st
  *     The details of the device found (output).
  * @param shortest_dev_path
  *     The path of the device found.  If nore than one is found, the
  *     shortest device_path is remembered.
  * @returns
  *     0 on success (shortest_dev_path has been set), -1 if the device
  *     is not found.
  */
static int
dev_stat_rec(explain_string_buffer_t *dev_path, dev_t dev,
    struct stat *st, explain_string_buffer_t *shortest_dev_path)
{
    DIR             *dp;
    int             pos;
    int             result;

    result = -1;
    dp = opendir(dev_path->message);
    if (!dp)
        return -1;
    pos = dev_path->position;
    for (;;)
    {
        struct dirent   *dep;
        struct stat     st2;

        dep = readdir(dp);
        if (!dep)
            break;
        if
        (
            dep->d_name[0] == '.'
        &&
            (
                dep->d_name[1] == '\0'
            ||
                (dep->d_name[1] == '.' && dep->d_name[2] == '\0')
            )
        )
            continue;
        dev_path->position = pos;
        dev_path->message[pos] = '\0';
        explain_string_buffer_path_join(dev_path, dep->d_name);
        if (lstat(dev_path->message, &st2) >= 0)
        {
            switch (st2.st_mode & S_IFMT)
            {
            case S_IFDIR:
                if (dev_stat_rec(dev_path, dev, st, shortest_dev_path) >= 0)
                {
                    result = 0;
                }
                break;

            case S_IFBLK:
            case S_IFCHR:
                if (dev == st2.st_rdev)
                {
                    if
                    (
                        !shortest_dev_path->position
                    ||
                        dev_path->position < shortest_dev_path->position
                    )
                    {
                        shortest_dev_path->position = 0;
                        *st = st2;
                        explain_string_buffer_write
                        (
                            shortest_dev_path,
                            dev_path->message,
                            dev_path->position
                        );
                    }
                    result = 0;
                }
                break;

            default:
                /* ignore everything else */
                break;
            }
        }
    }
    closedir(dp);
    dev_path->position = pos;
    dev_path->message[pos] = '\0';
    return result;
}


int
explain_buffer_device_name(explain_string_buffer_t *sb, dev_t dev,
    struct stat *st)
{
    explain_string_buffer_t dev_path_buf;
    char            dev_path[PATH_MAX + 1];

    explain_string_buffer_init(&dev_path_buf, dev_path, sizeof(dev_path));
    explain_string_buffer_puts(&dev_path_buf, "/dev");
    return dev_stat_rec(&dev_path_buf, dev, st, sb);
}


/* vim: set ts=8 sw=4 et : */
