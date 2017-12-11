/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_GROUP_IN_GROUPS_H
#define LIBEXPLAIN_GROUP_IN_GROUPS_H

struct explain_have_identity_t; /* forward */

/**
  * The explain_group_in_groups function may be used to determine
  * whether or not the given geoup matches the process' effective GID,
  * or is in the process' groups list.
  *
  * @param gid
  *    The group ID to test
  * @param hip
  *    The identity to check against
  * @returns
  *    int; non-zero (true) if in groups, zero (false) if not
  */
int explain_group_in_groups(int gid,
    const struct explain_have_identity_t *hip);

#endif /* LIBEXPLAIN_GROUP_IN_GROUPS_H */
/* vim: set ts=8 sw=4 et : */
