/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013, 2014 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/acl/libacl.h>
#include <libexplain/ac/sys/acl.h>

#ifndef HAVE_ACL_ENTRIES

int
acl_entries(acl_t acl)
{
    int result = 0;
    acl_entry_t ent;
    int id = ACL_FIRST_ENTRY;
    for (;;)
    {
        if (0 == acl_get_entry(acl, id, &ent))
            break;
        ++result;
        id = ACL_NEXT_ENTRY;
    }
    return result;
}

#endif

/* vim: set ts=8 sw=4 et : */
