/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/is_efault.h>
#include <libexplain/stream_to_fildes.h>


int
explain_stream_to_fildes(FILE *fp)
{
    /*
     * The Linux fclose(3) man page says
     *
     *     "RETURN VALUE:  Upon successful completion 0 is returned.
     *     Otherwise, EOF is returned and the global variable errno is
     *     set to indicate the error.  In either case any further access
     *     (including another call to fclose()) to the stream results in
     *     undefined behavior."
     *
     * which is interesting because if close(2) fails, the file
     * descriptor is usually still open.  Thus, we make an attempt
     * to recover the file descriptor, to see if we can produce some
     * additional information.
     *
     * If you are using glibc you are plain out of luck, because
     * it very carefully assigns -1 to the file descriptor member.
     * Other implementations may not be so careful, indeed other
     * implementations may keep the FILE pointer valid if the underlying
     * file descriptor is still valid.
     */
    if (explain_is_efault_pointer(fp, sizeof(*fp)))
        return -1;
    return fileno(fp);
}


/* vim: set ts=8 sw=4 et : */
