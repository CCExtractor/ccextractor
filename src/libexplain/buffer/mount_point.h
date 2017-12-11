/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_MOUNT_POINT_H
#define LIBEXPLAIN_BUFFER_MOUNT_POINT_H

#include <libexplain/ac/sys/stat.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_mount_point function may be used to insert the
  * mount point of the file system the path is within.
  *
  * @param sb
  *     The string buffer to write to.
  * @param path
  *     The path of the file in the file system we care about.
  * @returns
  *     0 on success, -1 if no mount point inserted
  */
int explain_buffer_mount_point(explain_string_buffer_t *sb,
    const char *path);

/**
  * The explain_buffer_mount_point function may be used to insert the
  * mount point of the file system the dirname(path) is within.
  *
  * @param sb
  *     The string buffer to write to.
  * @param path
  *     The path of the file in the file system we care about.
  * @returns
  *     0 on success, -1 if no mount point inserted
  */
int explain_buffer_mount_point_dirname(explain_string_buffer_t *sb,
    const char *path);

/**
  * The explain_buffer_mount_point_fd function may be used to insert
  * the mount point of the file system the file descriptor is within.
  *
  * @param sb
  *     The string buffer to write to.
  * @param fildes
  *     The file descriptor of the file we care about.
  * @returns
  *     0 on success, -1 if no mount point inserted
  */
int explain_buffer_mount_point_fd(explain_string_buffer_t *sb,
    int fildes);

/**
  * The explain_buffer_mount_point_stat function may be used to insert
  * the mount point of the file system, as described by the stat struct.
  *
  * @param sb
  *     The string buffer to write to.
  * @param st
  *     The stat struct describing the file we care about.
  * @returns
  *     0 on success, -1 if no mount point inserted
  */
int explain_buffer_mount_point_stat(explain_string_buffer_t *sb,
    const struct stat *st);

/**
  * The explain_buffer_mount_point_dev function may be used to insert
  * the mount point of the file system, identified by the mounted device.
  *
  * @param sb
  *     The string buffer to write to.
  * @param dev
  *     The the mounted device to look for.
  * @returns
  *     0 on success, -1 if no mount point inserted
  */
int explain_buffer_mount_point_dev(explain_string_buffer_t *sb, dev_t dev);

/**
  * The explain_mount_point_noexec function may be used to test whether
  * or not a file system has been mounted with the "noexec" option.
  *
  * @param pathname
  *     The pathname of the file of interest
  * @returns
  *     int; zon-zero (true) if the "noexec" mount option is used,
  *     zero (false) if not.
  */
int explain_mount_point_noexec(const char *pathname);

/**
  * The explain_mount_point_nosuid function may be used to test whether
  * or not a file system has been mounted with the "nosuid" option.
  *
  * @param pathname
  *     The pathname of the file of interest
  * @returns
  *     int; zon-zero (true) if the "nosuid" mount option is used,
  *     zero (false) if not.
  */
int explain_mount_point_nosuid(const char *pathname);

struct stat; /* forward */

/**
  * The explain_mount_point_nodev function may be used to test whether
  * or not a file system has been mounted with the "nodev" option.
  *
  * @param pathname
  *     The pathname of the file of interest
  * @returns
  *     int; zon-zero (true) if the "nodev" mount option is used,
  *     zero (false) if not.
  */
int explain_mount_point_nodev(const struct stat *pathname);

#endif /* LIBEXPLAIN_BUFFER_MOUNT_POINT_H */
/* vim: set ts=8 sw=4 et : */
