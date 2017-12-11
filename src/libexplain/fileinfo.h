/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_FILEINFO_H
#define LIBEXPLAIN_FILEINFO_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_fileinfo_pid_fd_n function may be used to obtain the
  * path name of the file corresponding to the given process and given
  * file descriptor.
  *
  * @param pid
  *     The process ID of interest.
  * @param fildes
  *     The file descriptor of interest.
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_pid_fd_n(pid_t pid, int fildes, char *data,
    size_t data_size);

/**
  * The explain_fileinfo_self_fd_n function may be used to obtain the
  * path name of the file corresponding to the given file descriptor.
  *
  * @param fildes
  *     The file descriptor of interest.
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_self_fd_n(int fildes, char *data, size_t data_size);

/**
  * The explain_fileinfo_pid_cwd function may be used to obtain the
  * absolute path of the current working directory of the given process.
  *
  * @param pid
  *     The process ID of the process of interest.
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_pid_cwd(pid_t pid, char *data, size_t data_size);

/**
  * The explain_fileinfo_self_cwd function may be used to obtain the
  * absolute path of the current working directory of current process.
  *
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_self_cwd(char *data, size_t data_size);

/**
  * The explain_fileinfo_pid_exe function may be used to obtain the
  * absolute path of the executable file of the given process.
  *
  * @param pid
  *     The process ID of the process of interest.
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_pid_exe(pid_t pid, char *data, size_t data_size);

/**
  * The explain_fileinfo_self_exe function may be used to obtain the
  * absolute path of the executable file of the executing process.
  *
  * @param data
  *     Where to return the path.
  * @param data_size
  *     The maximum size of the returned path.
  * @returns
  *     true (non-zero) on success, or false (zero) on error
  */
int explain_fileinfo_self_exe(char *data, size_t data_size);

/**
  * The explain_fileinfo_dir_tree_in_use function is used to determine
  * whether or not a given mount point still has files open.
  *
  * @param dirpath
  *     The absolute path of the diretory to be checked.
  * @returns
  *     -1 on error, 0 i niot in use, 1 if in use at least once
  */
int explain_fileinfo_dir_tree_in_use(const char *dirpath);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_FILEINFO_H */
