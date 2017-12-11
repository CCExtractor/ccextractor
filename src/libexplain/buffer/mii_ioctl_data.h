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

#ifndef LIBEXPLAIN_BUFFER_MII_IOCTL_DATA_H
#define LIBEXPLAIN_BUFFER_MII_IOCTL_DATA_H

#include <libexplain/string_buffer.h>

struct mii_ioctl_data; /* forward */

/**
  * The explain_buffer_mii_ioctl_data function may be used to
  * print a representation of a mii_ioctl_data structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The mii_ioctl_data structure to be printed.
  */
void explain_buffer_mii_ioctl_data(explain_string_buffer_t *sb,
    const struct mii_ioctl_data *data);

#endif /* LIBEXPLAIN_BUFFER_MII_IOCTL_DATA_H */
/* vim: set ts=8 sw=4 et : */
