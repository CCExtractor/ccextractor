/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_IDE_TASK_REQUEST_T_H
#define LIBEXPLAIN_BUFFER_IDE_TASK_REQUEST_T_H

#include <libexplain/ac/linux/hdreg.h>

#include <libexplain/string_buffer.h>

#ifdef HAVE_LINUX_HDREG_H

/**
  * The explain_buffer_ide_task_request_t function may be used
  * to print a representation of a ide_task_request_t structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The ide_task_request_t structure to be printed.
  */
void explain_buffer_ide_task_request_t(explain_string_buffer_t *sb,
    const ide_task_request_t *data);

#else

void explain_buffer_ide_task_request_t(explain_string_buffer_t *sb,
    const void *data);

#endif

#endif /* LIBEXPLAIN_BUFFER_IDE_TASK_REQUEST_T_H */
/* vim: set ts=8 sw=4 et : */
