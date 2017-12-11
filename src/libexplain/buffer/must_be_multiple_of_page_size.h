/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_MUST_BE_MULTIPLE_OF_PAGE_SIZE_H
#define LIBEXPLAIN_BUFFER_MUST_BE_MULTIPLE_OF_PAGE_SIZE_H

#include <libexplain/string_buffer.h>

struct must_be_multiple_of_page_size; /* forward */

/**
  * The explain_buffer_must_be_multiple_of_page_size function may be used
  * to print a representation of a must_be_multiple_of_page_size structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param caption
  *     The name of the offending system call argument.
  */
void explain_buffer_must_be_multiple_of_page_size(explain_string_buffer_t *sb,
    const char *caption);

#endif /* LIBEXPLAIN_BUFFER_MUST_BE_MULTIPLE_OF_PAGE_SIZE_H */
/* vim: set ts=8 sw=4 et : */
