/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_IOCONTROL_TABLE_H
#define LIBEXPLAIN_IOCONTROL_TABLE_H

#include <libexplain/ac/stddef.h>

#include <libexplain/iocontrol.h>

/**
  * The explain_iocontrol_table global array variable is used to store
  * pointers to the information about each ioctl request.
  */
extern const explain_iocontrol_t *const explain_iocontrol_table[];

/**
  * The explain_iocontrol_table_size global variable is used to store
  * the number of entries in the explain_iocontrol_table array..
  */
extern const size_t explain_iocontrol_table_size;

#endif /* LIBEXPLAIN_IOCONTROL_TABLE_H */
/* vim: set ts=8 sw=4 et : */
