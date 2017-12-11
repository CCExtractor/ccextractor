/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013, 2014 Peter Miller
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

#include <libexplain/ac/limits.h>
#include <libexplain/ac/sys/param.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/check_fildes_range.h>
#include <libexplain/option.h>


static int
get_open_max(void)
{
    int open_max = sysconf(_SC_OPEN_MAX);
    if (open_max < 0)
    {
#ifdef OPEN_MAX
        open_max = OPEN_MAX; /* <sys/param.h> */
#else
        open_max = NOFILE; /* <sys/param.h> */
#endif
    }
    return open_max;
}


static void
whinge(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a file descriptor
         * is detected that is negative, or larger than
         * sysconf(_SC_OPEN_MAX).
         *
         * The range will be printed separately, if available, so do not
         * mention sysconf(_SC_OPEN_MAX) in the translation.
         */
        i18n("the %s argument is outside the allowed range for file "
            "descriptors"),
        caption
    );

    if (explain_option_dialect_specific())
    {
        int open_max = get_open_max();
        explain_string_buffer_printf(sb, " (0..%d)", open_max - 1);
    }
}


int
explain_buffer_check_fildes_range(explain_string_buffer_t *sb,
    int fildes, const char *caption)
{
    int open_max = get_open_max();
    if (fildes < 0 || fildes >= open_max)
    {
        whinge(sb, caption);
        return 0;
    }

    /*
     * No error, yay!
     * Except that we wanted to find one.
     */
    return -1;
}


/* vim: set ts=8 sw=4 et : */
