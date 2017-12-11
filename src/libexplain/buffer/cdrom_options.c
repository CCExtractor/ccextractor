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

#include <libexplain/ac/linux/cdrom.h>

#include <libexplain/buffer/cdrom_options.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef CDO_AUTO_CLOSE
    { "CDO_AUTO_CLOSE", CDO_AUTO_CLOSE },
#endif
#ifdef CDO_AUTO_EJECT
    { "CDO_AUTO_EJECT", CDO_AUTO_EJECT },
#endif
#ifdef CDO_USE_FFLAGS
    { "CDO_USE_FFLAGS", CDO_USE_FFLAGS },
#endif
#ifdef CDO_LOCK
    { "CDO_LOCK", CDO_LOCK },
#endif
#ifdef CDO_CHECK_TYPE
    { "CDO_CHECK_TYPE", CDO_CHECK_TYPE },
#endif
};


void
explain_buffer_cdrom_options(explain_string_buffer_t *sb, int data)
{
    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
