/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/linux/tiocl.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/tioclinux.h>
#include <libexplain/iocontrol/tioclinux.h>


#ifdef TIOCLINUX

static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_tioclinux(sb, data);
}


/*
 * Bletch!
 *
 * This should have been about a dozen separate ioctl requests.
 * Someone needs to read Rusty's hierarchy of API goodness.
 * http://ozlabs.org/~rusty/index.cgi/tech/2008-03-30.html
 * http://www.pointy-stick.com/blog/2008/01/09/api-design-rusty-levels/
 * http://ozlabs.org/~rusty/ols-2003-keynote/ols-keynote-2003.html
 */
union foo
{
    char a[1 + sizeof(struct tiocl_selection)];
    char b[4 + sizeof(struct { long dummy[8]; })];
    char c[1 + sizeof(char *)];
};

/*
 * There are several cases that return data BUT they overwrite the
 * control bytes, so we don't know at print_data_returned time which
 * code it was, and thuis can't print a representation.  Sigh.
 */

const explain_iocontrol_t explain_iocontrol_tioclinux =
{
    "TIOCLINUX", /* name */
    TIOCLINUX, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    0, /* print_explanation */
    0, /* print_data_returned */
    sizeof(union foo), /* data_size */
    "union foo *", /* data_type */
    IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE, /* flags */
    __FILE__,
    __LINE__,
};

#else

const explain_iocontrol_t explain_iocontrol_tioclinux =
{
    0, /* name */
    0, /* value */
    0, /* disambiguate */
    0, /* print_name */
    0, /* print_data */
    0, /* print_explanation */
    0, /* print_data_returned */
    0, /* data_size */
    0, /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#endif


/* vim: set ts=8 sw=4 et : */
