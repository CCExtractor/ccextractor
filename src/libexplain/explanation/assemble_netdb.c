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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/hstrerror.h>
#include <libexplain/explanation.h>
#include <libexplain/explanation/assemble_common.h>


void
explain_explanation_assemble_netdb(explain_explanation_t *exp,
    explain_string_buffer_t *result)
{
    char            errstr[100];
    explain_string_buffer_t errstr_sb;
    int             hold_errno;

    hold_errno = errno;
    if (exp->errnum == 0)
    {
        explain_explanation_assemble_common(exp, "", result);
        return;
    }

    explain_string_buffer_init(&errstr_sb, errstr, sizeof(errstr));
    explain_buffer_hstrerror(&errstr_sb, exp->errnum, hold_errno);

    explain_explanation_assemble_common(exp, errstr, result);
}


/* vim: set ts=8 sw=4 et : */
