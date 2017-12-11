/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_HAVE_PERMISSION_H
#define LIBEXPLAIN_HAVE_PERMISSION_H

#include <libexplain/string_buffer.h>

struct stat; /* forward */

typedef struct explain_have_identity_t explain_have_identity_t;
struct explain_have_identity_t
{
    int uid;
    int gid;
};

/**
  * The explain_have_identity_init function may be used to initialise
  * an ID to the process effective ID.
  *
  * @param id
  *     The identity to be initialised.
  */
void explain_have_identity_init(explain_have_identity_t *id);

/**
  * The explain_have_read_permission function may be used to test
  * whether or not the current process has read permissions on an inode.
  *
  * @param st
  *    stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *    int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_have_read_permission(const struct stat *st,
    const explain_have_identity_t *hip);

/**
  * The explain_explain_read_permission function may be used to
  * explain why the current process (does not) have read permissions on
  * an inode.
  *
  * @param sb
  *     The string buffer to print into.
  * @param st
  *     The stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *     int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_explain_read_permission(explain_string_buffer_t *sb,
    const struct stat *st, const explain_have_identity_t *hip);

/**
  * The explain_have_write_permission function may be used to test
  * whether or not the current process has write permissions on an
  * inode.
  *
  * @param st
  *    stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *    int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_have_write_permission(const struct stat *st,
    const explain_have_identity_t *hip);

/**
  * The explain_explain_write_permission function may be used to
  * explain why the current process (does not) have write permissions on
  * an inode.
  *
  * @param sb
  *     The string buffer to print into.
  * @param st
  *     The stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *     int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_explain_write_permission(explain_string_buffer_t *sb,
    const struct stat *st, const explain_have_identity_t *hip);

/**
  * The explain_have_execute_permission function may be used to test
  * whether or not the current process has execute permissions on an
  * inode.
  *
  * @param st
  *    stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *    int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_have_execute_permission(const struct stat *st,
    const explain_have_identity_t *hip);

/**
  * The explain_explain_execute_permission function may be used to
  * explain why the current process (does not) have execute permissions
  * on an inode.
  *
  * @param sb
  *     The string buffer to print into.
  * @param st
  *     The stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *     int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_explain_execute_permission(explain_string_buffer_t *sb,
    const struct stat *st, const explain_have_identity_t *hip);

/**
  * The explain_have_search_permission function may be used to test
  * whether or not the current process has search permissions on an
  * inode.
  *
  * @param st
  *    stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *    int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_have_search_permission(const struct stat *st,
    const explain_have_identity_t *hip);

/**
  * The explain_explain_search_permission function may be used to
  * explain why the current process (does not) have search permissions
  * on an inode.
  *
  * @param sb
  *     The string buffer to print into.
  * @param st
  *     The stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *     int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_explain_search_permission(explain_string_buffer_t *sb,
    const struct stat *st, const explain_have_identity_t *hip);

/**
  * The explain_have_inode_permission function may be used to test
  * whether or not the current process has inode changing permissions
  * (utimes, chmod, etc) to an inode.
  *
  * @param st
  *    stat structure containing information about the file.
  * @param hip
  *    The user/process identity to check against.
  * @returns
  *    int; nonzero(true) if have permission, zero (false) if not.
  */
int explain_have_inode_permission(const struct stat *st,
    const explain_have_identity_t *hip);

/**
  * The explain_have_identity_kind_of_uid is used to obtain a string
  * describing the kind of UID in the identity.
  *
  * @param hip
  *     The identity of interest
  * @returns
  *     One of "real UID" or "effective UID", suitably translated.
  */
const char *explain_have_identity_kind_of_uid(
    const explain_have_identity_t *hip);

/**
  * The explain_have_identity_kind_of_gid is used to obtain a string
  * describing the kind of GID in the identity.
  *
  * @param hip
  *     The identity of interest
  * @returns
  *     One of "real GID" or "effective GID", suitably translated.
  */
const char *explain_have_identity_kind_of_gid(
    const explain_have_identity_t *hip);

#endif /* LIBEXPLAIN_HAVE_PERMISSION_H */
/* vim: set ts=8 sw=4 et : */
