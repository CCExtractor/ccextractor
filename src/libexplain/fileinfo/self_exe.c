/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/ac/unistd.h>

#include <libexplain/fileinfo.h>


int
explain_fileinfo_self_exe(char *data, size_t data_size)
{
#ifdef PROC_SELF
    /*
     * FIXME: is it worth the trouble of using the /proc/self link?
     * Seems like twice the code for no significant benefit, and we
     * would have to fall back to the pid method anyway.
     */
#endif
    return explain_fileinfo_pid_exe(getpid(), data, data_size);
}


/* vim: set ts=8 sw=4 et : */
