/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_PATH_RESOLUTION_H
#define LIBEXPLAIN_BUFFER_ERRNO_PATH_RESOLUTION_H

#include <libexplain/have_permission.h>
#include <libexplain/string_buffer.h>

typedef struct explain_final_t explain_final_t;
struct explain_final_t
{
    unsigned        want_to_read    :1;
    unsigned        want_to_write   :1;
    unsigned        want_to_search  :1;
    unsigned        want_to_execute :1;
    unsigned        want_to_create  :1;
    unsigned        want_to_modify_inode :1;
    unsigned        want_to_unlink  :1;
    unsigned        must_exist      :1;
    unsigned        must_not_exist  :1;
    unsigned        follow_symlink  :1;

    /**
      * This is set to 0 by default, meaning any file type is
      * acceptable.  If you want a specific type, set this bit, and
      * also set the st_mode member (below) to the desired file type,
      * e.g. S_IFREG, S_IFDIR, etc.
      */
    unsigned        must_be_a_st_mode :1;

    /**
      * When want_to_execute is set, the executable file should be
      * checked for #! and if so, the named interpreter's path should
      * also be checked for executability.  (This is the default).  To
      * avoid infinite recursion, when checking the interpreter itself,
      * disable this flag.
      */
    unsigned        follow_interpreter :1;

    /**
      * When want_to_create, this is an indication of what is about to
      * be created, so that it can be used to inform the error messages.
      *
      * When want a file to exist (the must_be_a_st_mode bit is set)
      * this member indicates the desired file type.
      *
      * Defaults to S_IFREG.
      */
    unsigned        st_mode;

    /*
     * The specific UID and GID to test against.
     * Defaults to geteuid() and getegid().
     * Required by the access(2) system call.
     */
    explain_have_identity_t id;

    /*
     * The specific maximum path length to test against.
     * Defaults to -1, meaning use pathconf(_PC_PATH_MAX).
     * This is needed when explaining sockaddr_un.sun_path errors.
     */
    long            path_max;

    /*
     * When you add to this struct, be sure to add to the
     * explain_final_init() code which initialises the fields to
     * default settings.
     */
};

/**
  * The explain_final_init function is used to initialise all of the
  * members of a explain_final_t struct to their defaults (usually 0).
  *
  * @param final_component
  *     Pointer to struct to be initialised.
  */
void explain_final_init(explain_final_t *final_component);

/**
  * The explain_buffer_errno_path_resolution function may be used to
  * check a path for problems.
  *
  * @param sb
  *    The string buffer to write the error to, once it is found
  * @param errnum
  *    The error number expected.
  * @param pathname
  *    The path being checked.
  * @param pathname_caption
  *    The name of the argument being checked in the function arguments
  *    of the system call being deciphered.
  * @param final_component
  *    Flags controlling the final component
  * @returns
  *    0 on success, meaning it found an error and wrote it to sb;
  *    -1 on failure, meaning it didn't find an error or it didn't find
  *    the expected error.
  */
int explain_buffer_errno_path_resolution(explain_string_buffer_t *sb,
    int errnum, const char *pathname, const char *pathname_caption,
    const explain_final_t *final_component);

#define must_be_a_directory(fcp) \
    (fcp->must_be_a_st_mode && fcp->st_mode == S_IFDIR)

/**
  * The explain_buffer_errno_path_resolution_at function may be used to
  * check a path for problems.
  *
  * @param sb
  *    The string buffer to write the error to, once it is found
  * @param errnum
  *    The error number expected.
  * @param fildes
  *    The directory file descriptor that relative file names are relative to.
  * @param pathname
  *    The path being checked.
  * @param pathname_caption
  *    The name of the argument being checked in the function arguments
  *    of the system call being deciphered.
  * @param final_component
  *    Flags controlling the final component
  * @returns
  *    0 on success, meaning it found an error and wrote it to sb;
  *    -1 on failure, meaning it didn't find an error or it didn't find
  *    the expected error.
  */
int explain_buffer_errno_path_resolution_at(explain_string_buffer_t *sb,
    int errnum, int fildes, const char *pathname, const char *pathname_caption,
    const explain_final_t *final_component);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_PATH_RESOLUTION_H */
/* vim: set ts=8 sw=4 et : */
