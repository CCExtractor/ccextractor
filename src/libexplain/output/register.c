/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/ac/stdlib.h>

#include <libexplain/output.h>
#include <libexplain/output/stderr.h>

static explain_output_t *where;


void
explain_output_message(const char *text)
{
    if (!where)
        where = &explain_output_static_stderr;
    explain_output_method_message(where, text);
}


void
explain_output_exit(int status)
{
    if (!where)
        where = &explain_output_static_stderr;
    explain_output_method_exit(where, status);
}


void
explain_output_exit_failure(void)
{
    explain_output_exit(EXIT_FAILURE);
}


void
explain_output_register(explain_output_t *op)
{
    if (op == where)
        return;
    explain_output_method_destructor(where);
    where = op;
}


/* vim: set ts=8 sw=4 et : */
