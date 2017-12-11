/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/buffer/acl_type.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/ac/sys/acl.h>
#include <libexplain/parse_bits.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef ACL_TYPE_ACCESS
    { "ACL_TYPE_ACCESS", ACL_TYPE_ACCESS },
#endif
#ifdef ACL_TYPE_DEFAULT
    { "ACL_TYPE_DEFAULT", ACL_TYPE_DEFAULT },
#endif
#ifdef ACL_TYPE_EXTENDED
    { "ACL_TYPE_EXTENDED", ACL_TYPE_EXTENDED },
#endif
#ifdef ACL_TYPE_NFS_v4
    { "ACL_TYPE_NFS_v4", ACL_TYPE_NFSv4 },
#endif
};


void
explain_buffer_acl_type(explain_string_buffer_t *sb, acl_type_t value)
{
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


acl_type_t
explain_acl_type_parse(const char *text)
{
    int             result;

    result = -1;
    explain_parse_bits(text, table, SIZEOF(table), &result);
    return (acl_type_t)result;
}


acl_type_t
explain_acl_type_parse_or_die(const char *text, const char *caption)
{
    int result;

    result = explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
    return (acl_type_t)result;
}


/* vim: set ts=8 sw=4 et : */
