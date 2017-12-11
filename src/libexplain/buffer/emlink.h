/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_EMLINK_H
#define LIBEXPLAIN_BUFFER_EMLINK_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_emlink function may be used to
  * print an explanation of an EMLINK error into the given buffer.
  *
  * @param sb
  *    The string buffer to print on.
  * @param oldpath
  *    The original oldpath, exactly as passed to system call.
  * @param newpath
  *    The original newpath, exactly as passed to system call.
  */
void explain_buffer_emlink(explain_string_buffer_t *sb,
    const char *oldpath, const char *newpath);

/**
  * The explain_buffer_emlink_mkdir function may be used to print an
  * explanation of an EMLINK error, in the case where mkdir cannot
  * proceed because the parent directory has too many links (would
  * exceed LINK_MAX).
  *
  * @param sb
  *    The string buffer to print on.
  * @param parent
  *    The path of the directory that has too many links.
  * @param argument_name
  *    The name of the offending argument.
  */
void explain_buffer_emlink_mkdir(explain_string_buffer_t *sb,
    const char *parent, const char *argument_name);

#endif /* LIBEXPLAIN_BUFFER_EMLINK_H */
/* vim: set ts=8 sw=4 et : */
