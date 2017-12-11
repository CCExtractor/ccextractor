/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/fcntl.h>

#include <libexplain/string_buffer.h>
#include <libexplain/string_flags.h>


void
explain_string_flags_init(explain_string_flags_t *sf, const char *flags)
{
    int             acc_mode;
    int             options;
    const char      *cp;
    explain_string_buffer_t yuck_buf;
    int             plus_seen;

    explain_string_buffer_init(&yuck_buf, sf->invalid, sizeof(sf->invalid));

    /*
     * Parse the flags string.
     *
     * (It turns out glibc is more generous than this, when it comes to
     * validity, but we only complain for EINVAL.  Different systems
     * will see validity differently.)
     */
    sf->rwa_seen = 0;
    acc_mode = O_RDONLY;
    options = 0;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
#ifdef O_LARGEFILE_HIDDEN
    options |= O_LARGEFILE_HIDDEN;
#endif
    sf->flags_string_valid = 1;
    cp = flags;
    plus_seen = 0;
    for (;;)
    {
        unsigned char   c;

        c = *cp++;
        switch (c)
        {
        case '\0':
            /* end of string */
            if (!sf->rwa_seen)
                sf->flags_string_valid = 0;
            sf->flags = acc_mode | options;
            return;

        case '+':
            /*
             * "r+" Open for reading and writing.  The stream is
             *      positioned at the beginning of the file.
             *
             * "w+" Open for reading and writing.  The file is created
             *      if it does not exist, otherwise it is truncated.
             *      The stream is positioned at the beginning of the
             *      file.
             *
             * "a+" Open for reading and appending (writing at end of
             *      file).  The file is created if it does not exist.
             *      The initial file position for reading is at the
             *      beginning of the file, but output is always appended
             *      to the end of the file.
             */
            if (!sf->rwa_seen || plus_seen)
                goto duplicate;
            acc_mode = O_RDWR;
            plus_seen = 1;
            break;

        case 'a':
            /*
             * Open for appending (writing at end of file).  The file is
             * created if it does not exist.  The stream is positioned
             * at the end of the file.
             */
            if (sf->rwa_seen && (options & O_APPEND))
                goto duplicate;
            acc_mode = O_WRONLY;
            options |= O_CREAT | O_APPEND;
            sf->rwa_seen = 1;
            break;

        case 'b':
            /*
             * The mode string can also include the letter 'b' either
             * as a last character or as a character between the
             * characters in any of the two-character strings
             * described above.  This is strictly for compatibility with
             * C89 and has no effect; the 'b' is ignored on all POSIX
             * conforming systems, including Linux.  (Other systems
             * may treat text files and binary files differently, and
             * adding the 'b' may be a good idea if you do I/O to a
             * binary file and expect that your program may be ported to
             * non-UNIX environments.)
             */
            options |= O_BINARY;
            break;

        case  'c':
            /*
             * GNU extension
             * not a thread cancellation point
             */
            break;

        case 'e':
            /* GNU extension */
            options |= O_CLOEXEC;
            break;

        case 'm':
            /*
             * GNU extension
             * try to use mmap
             */
            break;

        case 'r':
            /*
             * Open text file for reading.  The stream is positioned at
             * the beginning of the file.
             */
            if (sf->rwa_seen && acc_mode == O_RDONLY)
                goto duplicate;
            acc_mode = O_RDONLY;
            sf->rwa_seen = 1;
            break;

        case 't':
            /* GNU extension */
            options |= O_TEXT;
            break;

        case 'w':
            /*
             * Truncate file to zero length or create text file for
             * writing.  The stream is positioned at the beginning of
             * the file.
             */
            if (sf->rwa_seen && acc_mode == O_WRONLY)
                goto duplicate;
            acc_mode = O_WRONLY;
            options |= O_CREAT | O_TRUNC;
            sf->rwa_seen = 1;
            break;

        case 'x':
            /* GNU extension */
            options |= O_EXCL;
            break;

        default:
            duplicate:
            sf->flags_string_valid = 0;
            explain_string_buffer_putc(&yuck_buf, c);
            break;
        }
    }
}

/* vim: set ts=8 sw=4 et : */
