/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/pathconf_name.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef _PC_LINK_MAX
    { "_PC_LINK_MAX", _PC_LINK_MAX },
#endif
#ifdef _PC_MAX_CANON
    { "_PC_MAX_CANON", _PC_MAX_CANON },
#endif
#ifdef _PC_MAX_INPUT
    { "_PC_MAX_INPUT", _PC_MAX_INPUT },
#endif
#ifdef _PC_NAME_MAX
    { "_PC_NAME_MAX", _PC_NAME_MAX },
#endif
#ifdef _PC_PATH_MAX
    { "_PC_PATH_MAX", _PC_PATH_MAX },
#endif
#ifdef _PC_PIPE_BUF
    { "_PC_PIPE_BUF", _PC_PIPE_BUF },
#endif
#ifdef _PC_CHOWN_RESTRICTED
    { "_PC_CHOWN_RESTRICTED", _PC_CHOWN_RESTRICTED },
#endif
#ifdef _PC_NO_TRUNC
    { "_PC_NO_TRUNC", _PC_NO_TRUNC },
#endif
#ifdef _PC_VDISABLE
    { "_PC_VDISABLE", _PC_VDISABLE },
#endif
#ifdef _PC_SYNC_IO
    { "_PC_SYNC_IO", _PC_SYNC_IO },
#endif
#ifdef _PC_ASYNC_IO
    { "_PC_ASYNC_IO", _PC_ASYNC_IO },
#endif
#ifdef _PC_PRIO_IO
    { "_PC_PRIO_IO", _PC_PRIO_IO },
#endif
#ifdef _PC_SOCK_MAXBUF
    { "_PC_SOCK_MAXBUF", _PC_SOCK_MAXBUF },
#endif
#ifdef _PC_FILESIZEBITS
    { "_PC_FILESIZEBITS", _PC_FILESIZEBITS },
#endif
#ifdef _PC_REC_INCR_XFER_SIZE
    { "_PC_REC_INCR_XFER_SIZE", _PC_REC_INCR_XFER_SIZE },
#endif
#ifdef _PC_REC_MAX_XFER_SIZE
    { "_PC_REC_MAX_XFER_SIZE", _PC_REC_MAX_XFER_SIZE },
#endif
#ifdef _PC_REC_MIN_XFER_SIZE
    { "_PC_REC_MIN_XFER_SIZE", _PC_REC_MIN_XFER_SIZE },
#endif
#ifdef _PC_REC_XFER_ALIGN
    { "_PC_REC_XFER_ALIGN", _PC_REC_XFER_ALIGN },
#endif
#ifdef _PC_ALLOC_SIZE_MIN
    { "_PC_ALLOC_SIZE_MIN", _PC_ALLOC_SIZE_MIN },
#endif
#ifdef _PC_SYMLINK_MAX
    { "_PC_SYMLINK_MAX", _PC_SYMLINK_MAX },
#endif
#ifdef _PC_2_SYMLINKS
    { "_PC_2_SYMLINKS", _PC_2_SYMLINKS },
#endif
#ifdef _PC_MIN_HOLE_SIZE
    { "_PC_MIN_HOLE_SIZE", _PC_MIN_HOLE_SIZE },
#endif
};


void
explain_buffer_pathconf_name(explain_string_buffer_t *sb, int name)
{
    explain_parse_bits_print_single(sb, name, table, SIZEOF(table));
}


int
explain_parse_pathconf_name_or_die(const char *text)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), 0);
}


int
explain_valid_pathconf_name(int name)
{
    return !!explain_parse_bits_find_by_value(name, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
