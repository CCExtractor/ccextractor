/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>

#include <libexplain/uid_from_pid.h>


uid_t
explain_uid_from_pid(pid_t pid)
{
    char filename[100];
    FILE *fp;

    snprintf(filename, sizeof(filename), "/proc/%d/status", pid);
    fp = fopen(filename, "r");
    if (!fp)
        return (pid_t)-1;
    for (;;)
    {
        int ruid;
        int euid;
        int suid;
        char line[200];

        if (!fgets(line, sizeof(line), fp))
            break;
        if (0 != memcmp(line, "Uid:", 4))
            continue;
        if (3 == sscanf(line + 4, "%d%d%d", &ruid, &euid, &suid))
        {
            fclose(fp);
            return euid;
        }
    }
    fclose(fp);
    return (pid_t)-1;
}


/* vim: set ts=8 sw=4 et : */
