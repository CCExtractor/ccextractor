/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013, 2014 Peter Miller
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

#include <libexplain/ac/libio.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/is_efault.h>
#include <libexplain/libio.h>


int
explain_libio_no_reads(FILE *fp)
{
    if (explain_is_efault_pointer(fp, sizeof(*fp)))
        return 0;
#if defined(HAVE_LIBIO_H) && defined(_IO_NO_READS)
    return !!(((_IO_FILE *)fp)->_flags & _IO_NO_READS);
#elif defined(__BSD_VISIBLE) && defined(__SRD)
    return !((fp->_flags & __SRD) || (fp->_flags & __SRW));
#else
    (void)fp;
    return 0;
#endif
}


/* vim: set ts=8 sw=4 et : */
