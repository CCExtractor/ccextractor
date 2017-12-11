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

#include <libexplain/explanation.h>


void
explain_explanation_init(explain_explanation_t *exp, int errnum)
{
    explain_string_buffer_init
    (
        &exp->system_call_sb,
        exp->system_call,
        sizeof(exp->system_call)
    );
    exp->errnum = errnum;
    explain_string_buffer_init
    (
        &exp->explanation_sb,
        exp->explanation,
        sizeof(exp->explanation)
    );
    explain_string_buffer_init
    (
        &exp->footnotes_sb,
        exp->footnotes,
        sizeof(exp->footnotes)
    );
    exp->explanation_sb.footnotes = &exp->footnotes_sb;
    exp->system_call_sb.footnotes = &exp->footnotes_sb;
}


/* vim: set ts=8 sw=4 et : */
