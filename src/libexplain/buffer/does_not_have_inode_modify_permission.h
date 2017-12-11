/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_DOES_NOT_HAVE_INODE_MODIFY_PERMISSION_H
#define LIBEXPLAIN_BUFFER_DOES_NOT_HAVE_INODE_MODIFY_PERMISSION_H

#include <libexplain/string_buffer.h>

struct stat; /* forward */
struct explain_have_identity_t; /* forward */

/**
  * The explain_buffer_does_not_have_inode_modify_permission function
  * may be used to provide an explanation in the case where a process
  * does not have sufficent permissions to modify an inode.
  *
  * @param sb
  *    The string buffer to print into.
  * @param comp
  *    The name of the problematic component
  * @param comp_st
  *    The file status of the problemtic component
  * @param caption
  *    The name of the problematic system call argument
  * @param dir
  *    The pathname of the directory containing the problematic component
  * @param dir_st
  *    The file status of the directory containing the problematic component
  * @param hip
  *    The process ID for this operation.
  */
void explain_buffer_does_not_have_inode_modify_permission(
    explain_string_buffer_t *sb, const char *comp,
    const struct stat *comp_st, const char *caption, const char *dir,
    const struct stat *dir_st, const struct explain_have_identity_t *hip);

/**
  * The explain_buffer_does_not_have_inode_modify_permission1 function
  * may be used to provide an explanation in the case where a process
  * does not have sufficent permissions to modify an inode.
  *
  * @param sb
  *    The string buffer to print into.
  * @param pathname
  *    The pathname of the problematic component
  * @param pathname_st
  *    The file status of the problematic component
  * @param caption
  *    The name of the problematic system call argument
  * @param hip
  *    The process ID for this operation.
  */
void explain_buffer_does_not_have_inode_modify_permission1(
    explain_string_buffer_t *sb, const char *pathname,
    const struct stat *pathname_st, const char *caption,
    const struct explain_have_identity_t *hip);

/**
  * The explain_buffer_does_not_have_inode_modify_permission_fd
  * function may be used to provide an explanation in the case where a
  * process does not have sufficent permissions to modify an inode, as
  * given by a file descriptor.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The problematic file descriptor
  * @param fildes_caption
  *    The name of the problematic system call argument
  */
void explain_buffer_does_not_have_inode_modify_permission_fd(
    explain_string_buffer_t *sb, int fildes, const char *fildes_caption);

/**
  * The explain_buffer_does_not_have_inode_modify_permission_fd_st
  * function may be used to provide an explanation in the case where a
  * process does not have sufficent permissions to modify an inode, as
  * given by a file descriptor.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes_st
  *    The file status of the problematic file descriptor
  * @param fildes_caption
  *    The name of the problematic system call argument
  * @param hip
  *    The process ID for this operation.
  */
void explain_buffer_does_not_have_inode_modify_permission_fd_st(
    explain_string_buffer_t *sb, const struct stat *fildes_st,
    const char *fildes_caption, const struct explain_have_identity_t *hip);

#endif /* LIBEXPLAIN_BUFFER_DOES_NOT_HAVE_INODE_MODIFY_PERMISSION_H */
/* vim: set ts=8 sw=4 et : */
