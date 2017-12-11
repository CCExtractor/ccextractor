/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010-2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/option.h>
#include <libexplain/output.h>
#include <libexplain/program_name.h>
#include <libexplain/string_buffer.h>


void
explain_output_error(const char *format, ...)
{
    va_list         ap;
    explain_string_buffer_t sb;

    /*
     * Note: we can't use explain_common_message_buffer, just in case
     * one of the format argumnets *is* explain_common_message_buffer.
     * And for the same reason, we need to be about the same size.
     */
    char buf[PATH_MAX * 2 + 200];

    /*
     * See if we can just pass the text through, unchanged.
     */
    if (0 == strcmp(format, "%s") && !explain_option_assemble_program_name())
    {
        const char      *text;

        va_start(ap, format);
        text = va_arg(ap, const char *);
        va_end(ap);
        explain_output_message(text);
        return;
    }

    explain_string_buffer_init(&sb, buf, sizeof(buf));
    if (explain_option_assemble_program_name())
    {
        const char      *prog;

        prog = explain_program_name_get();
        if (prog && *prog)
        {
            explain_string_buffer_puts(&sb, prog);
            explain_string_buffer_puts(&sb, ": ");
        }
    }

    va_start(ap, format);
    explain_string_buffer_vprintf(&sb, format, ap);
    va_end(ap);

    explain_output_message(buf);
}


/* vim: set ts=8 sw=4 et : */
