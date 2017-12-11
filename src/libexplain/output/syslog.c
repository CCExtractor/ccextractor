/*
 * libexplain - Explain errno values returned by libc functions
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

#include <libexplain/ac/string.h>
#include <libexplain/ac/syslog.h>

#include <libexplain/output.h>
#include <libexplain/program_name.h>


typedef struct explain_output_syslog_t explain_output_syslog_t;
struct explain_output_syslog_t
{
    explain_output_t inherited;
    int priority;
};

static int openlog_has_been_called;


static void
message(explain_output_t *op, const char *text)
{
    explain_output_syslog_t *p;
    const char      *prog;
    size_t          text_size;

    text_size = strlen(text);
    while (text_size > 0 && text[text_size - 1] == '\n')
        --text_size;

    /*
     * The syslog implementation in glibc is determined to have the "ident"
     * in the error message.  Contrary to the man page, if "ident" is NULL,
     * instead of no program name (as documented), it inserts its own idea of
     * the program name.  To have it use libexplain's idea of the program
     * name, we provide it to openlog(), and remove it (if present) from the
     * string passed to syslog().  Plus, an empty "ident" string makes for an
     * inconsistent {ugly} syslog format.
     */
    prog = explain_program_name_get();
    if (prog && *prog)
    {
        size_t prog_size = strlen(prog);
        size_t delta = prog_size + 2;
        if
        (
            text_size >= delta
        &&
            memcmp(text, prog, prog_size) == 0
        &&
            memcmp(text + prog_size, ": ", 2) == 0
        )
        {
            text += delta;
            text_size -= delta;
        }
    }
    if (text_size == 0)
        return;

    p = (explain_output_syslog_t *)op;
    syslog(p->priority, "%.*s\n", (int)text_size, text);
}


static const explain_output_vtable_t vtable =
{
    0, /* destructor */
    message,
    0, /* exit */
    sizeof(explain_output_t)
};


explain_output_t *
explain_output_syslog_new(void)
{
    int level = LOG_ERR;
    return explain_output_syslog_new1(level);
}


explain_output_t *
explain_output_syslog_new1(int level)
{
    int option = 0;
    int facility = LOG_USER;
    return explain_output_syslog_new3(option, facility, level);
}


explain_output_t *
explain_output_syslog_new3(int option, int facility, int level)
{
    explain_output_t *result;

    result = explain_output_new(&vtable);
    if (result)
    {
        explain_output_syslog_t *p;
        const char      *ident;

        /*
         * The syslog implementation in glibc is determined to have the "ident"
         * in the error message.  Contrary to the man page, if "ident" is NULL,
         * instead of no program name (as documented), it inserts its own idea
         * of the program name.  To have it use libexplain's idea of the
         * program name, we provide it to openlog(), and remove it (if present)
         * from the string passed to syslog().  Plus, an empty "ident" string
         * makes for an inconsistent {ugly} syslog format.
         */
        ident = explain_program_name_get();
        if (ident && !*ident)
            ident = NULL;

        if (!openlog_has_been_called)
        {
            openlog_has_been_called = 1;
            openlog(ident, option, facility);
        }
        p = (explain_output_syslog_t *)result;
        p->priority = facility | level;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
