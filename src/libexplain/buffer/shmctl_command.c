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

#include <libexplain/buffer/shmctl_command.h>
#include <libexplain/output.h>
#include <libexplain/parse_bits.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef IPC_STAT
    { "IPC_STAT", IPC_STAT },
#endif
#ifdef IPC_SET
    { "IPC_SET", IPC_SET },
#endif
#ifdef IPC_RMID
    { "IPC_RMID", IPC_RMID },
#endif
#ifdef IPC_INFO
    { "IPC_INFO", IPC_INFO },
#endif
#ifdef SHM_INFO
    { "SHM_INFO", SHM_INFO },
#endif
#ifdef SHM_STAT
    { "SHM_STAT", SHM_STAT },
#endif
#ifdef SHM_LOCK
    { "SHM_LOCK", SHM_LOCK },
#endif
#ifdef SHM_UNLOCK
    { "SHM_UNLOCK", SHM_UNLOCK },
#endif
};


void
explain_buffer_shmctl_command(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


int
explain_parse_shmctl_command_or_die(const char *text, const char *caption)
{
    int result = 0;
    if (explain_parse_bits(text, table, SIZEOF(table), &result) < 0)
    {
        char            part1[100];
        explain_string_buffer_t part1_sb;
        char            part2[1000];
        explain_string_buffer_t part2_sb;

        part1[0] = '\0';
        explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
        if (caption && *caption)
            explain_string_buffer_puts(&part1_sb, caption);
        explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
        explain_string_buffer_puts_quoted(&part2_sb, text);

        explain_output_error_and_die
        (
            "%s: unable to interpret %s as a shmctl(2) command: %s",
            part1,
            part2,
            explain_parse_bits_get_error()
        );
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
