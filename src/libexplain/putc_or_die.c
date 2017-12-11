/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/stdio.h>

#include <libexplain/putc.h>
#include <libexplain/output.h>


void
explain_putc_or_die_failed(int c, FILE *fp)
{
    explain_putc_on_error_failed(c, fp);
    explain_output_exit_failure();
}


#if __GNUC__ < 3

void
explain_putc_or_die(int c, FILE *fp)
{
    if (putc(c, fp) == EOF)
        explain_putc_or_die_failed(c, fp);
}

#endif


/* vim: set ts=8 sw=4 et : */
