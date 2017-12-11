/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_OTHERS_PERMISSION_IGNORED_H
#define LIBEXPLAIN_BUFFER_OTHERS_PERMISSION_IGNORED_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_others_permission_ignored function may be used
  * to print a message indicating the the "others" poertion of access
  * permission modes have been ignored.
  *
  * @param sb
  *     The string buffer to print into.
  * @param mode
  *     The permission mode bits in question.
  */
void explain_buffer_others_permission_ignored(explain_string_buffer_t *sb,
    int mode);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_BUFFER_OTHERS_PERMISSION_IGNORED_H */
