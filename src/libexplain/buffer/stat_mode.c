/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/stat_mode.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t hi_table[] =
{
    { "S_IFREG", S_IFREG },
    { "S_IFBLK", S_IFBLK },
    { "S_IFCHR", S_IFCHR },
    { "S_IFDIR", S_IFDIR },
#ifdef S_IFCMP
    { "S_IFCMP", S_IFCMP },
#endif
#ifdef S_IFDOOR
    { "S_IFDOOR", S_IFDOOR },
#endif
#ifdef S_IFIFO
    { "S_IFIFO", S_IFIFO },
#endif
#ifdef S_IFLNK
    { "S_IFLNK", S_IFLNK },
#endif
#ifdef S_IFMPB
    { "S_IFMPB", S_IFMPB },
#endif
#ifdef S_IFMPC
    { "S_IFMPC", S_IFMPC },
#endif
#ifdef S_IFNAM
    { "S_IFNAM", S_IFNAM },
#endif
#ifdef S_IFNWK
    { "S_IFNWK", S_IFNWK },
#endif
#ifdef S_IFSOCK
    { "S_IFSOCK", S_IFSOCK },
#endif
#ifdef S_IFWHT
    { "S_IFWHT", S_IFWHT },
#endif
};

static const explain_parse_bits_table_t lo_table_short[] =
{
    { "S_ISUID", S_ISUID },
    { "S_ISGID", S_ISGID },
    { "S_ISVTX", S_ISVTX },
};

static const explain_parse_bits_table_t lo_table_long[] =
{
    { "S_ISUID", S_ISUID },
    { "S_ISGID", S_ISGID },
    { "S_ISVTX", S_ISVTX },
    { "S_IRWXU", S_IRWXU },
    { "S_IRUSR", S_IRUSR },
    { "S_IWUSR", S_IWUSR },
    { "S_IXUSR", S_IXUSR },
    { "S_IRWXG", S_IRWXG },
    { "S_IRGRP", S_IRGRP },
    { "S_IWGRP", S_IWGRP },
    { "S_IXGRP", S_IXGRP },
    { "S_IRWXO", S_IRWXO },
    { "S_IROTH", S_IROTH },
    { "S_IWOTH", S_IWOTH },
    { "S_IXOTH", S_IXOTH },
};


void
explain_buffer_stat_mode(explain_string_buffer_t *sb, mode_t mode)
{
    const explain_parse_bits_table_t *lo_table;
    const explain_parse_bits_table_t *tp;
    int             first;

    if (mode == 0)
    {
        explain_string_buffer_putc(sb, '0');
        return;
    }
    first = 1;
    for (tp = hi_table; tp < ENDOF(hi_table); ++tp)
    {
        if (tp->value == (int)(mode & S_IFMT))
        {
            explain_string_buffer_puts(sb, tp->name);
            first = 0;
            mode -= tp->value;
            break;
        }
    }

    lo_table = lo_table_short;
    if (explain_option_symbolic_mode_bits())
    {
        lo_table = lo_table_long;
    }
    for (tp = lo_table; tp < ENDOF(lo_table); ++tp)
    {
        if (tp->value != 0 && (int)(mode & tp->value) == tp->value)
        {
            if (!first)
                explain_string_buffer_puts(sb, " | ");
            explain_string_buffer_puts(sb, tp->name);
            first = 0;
            mode -= tp->value;
        }
    }
    if (mode != 0)
    {
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_printf(sb, "%#o", (int)mode);
    }
}


static const explain_parse_bits_table_t full_table[] =
{
    { "S_IFREG", S_IFREG },
    { "S_IFBLK", S_IFBLK },
    { "S_IFCHR", S_IFCHR },
    { "S_IFDIR", S_IFDIR },
#ifdef S_IFCMP
    { "S_IFCMP", S_IFCMP },
#endif
#ifdef S_IFDOOR
    { "S_IFDOOR", S_IFDOOR },
#endif
#ifdef S_IFIFO
    { "S_IFIFO", S_IFIFO },
#endif
#ifdef S_IFLNK
    { "S_IFLNK", S_IFLNK },
#endif
#ifdef S_IFMPB
    { "S_IFMPB", S_IFMPB },
#endif
#ifdef S_IFMPC
    { "S_IFMPC", S_IFMPC },
#endif
#ifdef S_IFNAM
    { "S_IFNAM", S_IFNAM },
#endif
#ifdef S_IFNWK
    { "S_IFNWK", S_IFNWK },
#endif
#ifdef S_IFSOCK
    { "S_IFSOCK", S_IFSOCK },
#endif
#ifdef S_IFWHT
    { "S_IFWHT", S_IFWHT },
#endif
    { "S_IFMT", S_IFMT },
    { "S_ISUID", S_ISUID },
    { "S_ISGID", S_ISGID },
    { "S_ISVTX", S_ISVTX },
    { "S_IRWXU", S_IRWXU },
    { "S_IRUSR", S_IRUSR },
    { "S_IWUSR", S_IWUSR },
    { "S_IXUSR", S_IXUSR },
    { "S_IRWXG", S_IRWXG },
    { "S_IRGRP", S_IRGRP },
    { "S_IWGRP", S_IWGRP },
    { "S_IXGRP", S_IXGRP },
    { "S_IRWXO", S_IRWXO },
    { "S_IROTH", S_IROTH },
    { "S_IWOTH", S_IWOTH },
    { "S_IXOTH", S_IXOTH },
};


int
explain_parse_stat_mode_or_die(const char *text, const char *caption)
{
    return
        explain_parse_bits_or_die
        (
            text,
            full_table,
            SIZEOF(full_table),
            caption
        );
}


/* vim: set ts=8 sw=4 et : */
