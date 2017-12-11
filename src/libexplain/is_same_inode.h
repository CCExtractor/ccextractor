/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_IS_SAME_INODE_H
#define LIBEXPLAIN_IS_SAME_INODE_H

struct stat; /* forward */

/**
  * The explain_is_same_inode function may be used to compare two
  * stat structs to determine whether or not they describe the same
  * inode.
  *
  * @param st1
  *    pointer to the first stst struct
  * @param st2
  *    pointer to the second stst struct
  * @returns
  *    int; 0 if not the same, 1 if the same
  */
int explain_is_same_inode(const struct stat *st1, const struct stat *st2);

#endif /* LIBEXPLAIN_IS_SAME_INODE_H */
/* vim: set ts=8 sw=4 et : */
