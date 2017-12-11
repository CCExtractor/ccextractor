/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/shm.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/shmflg.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>


void
explain_buffer_shmflg(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef SHM_RDONLY
        { "SHM_RDONLY", SHM_RDONLY },
#endif
#ifdef SHM_RND
        { "SHM_RND", SHM_RND },
#endif
#ifdef SHM_REMAP
        { "SHM_REMAP", SHM_REMAP },
#endif
#ifdef SHM_EXEC
        { "SHM_EXEC", SHM_EXEC },
#endif
#ifdef S_IRUSR
        { "S_IRUSR", S_IRUSR },
#endif
#ifdef S_IWUSR
        { "S_IWUSR", S_IWUSR },
#endif
#ifdef S_IXUSR
        { "S_IXUSR", S_IXUSR },
#endif
#ifdef S_IRGRP
        { "S_IRGRP", S_IRGRP },
#endif
#ifdef S_IWGRP
        { "S_IWGRP", S_IWGRP },
#endif
#ifdef S_IXGRP
        { "S_IXGRP", S_IXGRP },
#endif
#ifdef S_IROTH
        { "S_IROTH", S_IROTH },
#endif
#ifdef S_IWOTH
        { "S_IWOTH", S_IWOTH },
#endif
#ifdef S_IXOTH
        { "S_IXOTH", S_IXOTH },
#endif
    };

    if (explain_option_symbolic_mode_bits())
    {
        explain_parse_bits_print(sb, value, table, SIZEOF(table));
    }
    else
    {
        int             mode;

        mode = value & 0777;
        value &= ~0777;
        if (value)
        {
            explain_parse_bits_print(sb, value, table, SIZEOF(table));
            if (mode)
                explain_string_buffer_printf(sb, " | %04o", mode);
        }
        else
        {
            explain_string_buffer_printf(sb, "%04o", mode);
        }
    }
}


/* vim: set ts=8 sw=4 et : */
