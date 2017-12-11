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

#ifndef LIBEXPLAIN_ERRNO_INFO_H
#define LIBEXPLAIN_ERRNO_INFO_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_errno_info_t type describes an errno value, and its
  * (macro) name.
  *
  * While the tables contains a description string, it is only used if
  * the "internal-strerror=true" option is set.  It is principally of
  * use for automated testing, to elimitate differences between Unix
  * implimentations.
  */
typedef struct explain_errno_info_t explain_errno_info_t;
struct explain_errno_info_t
{
    int error_number;
    const char *name;
    const char *description;
};

/**
  * The explain_errno_info_by_number function may be used to locate
  * errno information by errno number.
  *
  * @param errnum
  *     The error number
  * @returns
  *     pointer to info on success, or NULL on failure
  */
const explain_errno_info_t *explain_errno_info_by_number(int errnum);

/**
  * The explain_errno_info_by_name function may be used to locate
  * errno information by errno name.
  *
  * @param name
  *     The error name ("ENOENT", etc)
  * @returns
  *     pointer to info on success, or NULL on failure
  */
const explain_errno_info_t *explain_errno_info_by_name(const char *name);

/**
  * The explain_errno_info_by_name_fuzzy function may be used to
  * locate errno information by errno name, using fuzzy matching.  (For
  * best results, try the exact match first.)
  *
  * @param name
  *     The error name ("ENOENT", etc)
  * @returns
  *     pointer to info on success, or NULL on failure
  */
const explain_errno_info_t *explain_errno_info_by_name_fuzzy(
    const char *name);

/**
  * The explain_errno_info_by_text function may be used to locate
  * errno information by errno text.
  *
  * @param text
  *     The error name ("No such file or directory", etc)
  * @returns
  *     pointer to info on success, or NULL on failure
  */
const explain_errno_info_t *explain_errno_info_by_text(const char *text);

/**
  * The explain_errno_info_by_text_fuzzy function may be used to
  * locate errno information by errno text, using fuzzy matching.  (For
  * best results, try the exact match first.)
  *
  * @param text
  *     The error text ("No such file or directory", etc)
  * @returns
  *     pointer to info on success, or NULL on failure
  */
const explain_errno_info_t *explain_errno_info_by_text_fuzzy(
    const char *text);

/**
  * The explain_internal_strerror method may be used to obtain
  * a string corresponding to an error number.  It honors the
  * #explain_option_internal_strerror flag.
  *
  * @returns
  *     on failure, NULL if the error number os unknown; or
  *     on success, a pointer to a string that may not bee altered or free()ed.
  */
const char *explain_internal_strerror(int n);

/**
  * The explain_internal_strerror_r method may be used to obtain
  * a string corresponding to an error number.  It honors the
  * #explain_option_internal_strerror flag.
  *
  * @param errnum
  *     The error number to describe
  * @param data
  *     The array in which to return the result.
  * @param data_size
  *     The maximum size of thereturned string, including the
  *     terminating NUL character.
  */
void explain_internal_strerror_r(int errnum, char *data, size_t data_size);

#endif /* LIBEXPLAIN_ERRNO_INFO_H */
/* vim: set ts=8 sw=4 et : */
