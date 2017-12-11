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

#include <libexplain/ac/stdio.h>

#include <libexplain/output.h>


typedef struct explain_output_file_t explain_output_file_t;
struct explain_output_file_t
{
    explain_output_t inherited;
    FILE            *fp;
};


static void
destructor(explain_output_t *op)
{
    explain_output_file_t *p;

    p = (explain_output_file_t *)op;
    fclose(p->fp);
}


static void
message(explain_output_t *op, const char *text)
{
    explain_output_file_t *p;

    p = (explain_output_file_t *)op;
    fprintf(p->fp, "%s\n", text);
}


static const explain_output_vtable_t vtable =
{
    destructor,
    message,
    0, /* exit */
    sizeof(explain_output_file_t)
};


explain_output_t *
explain_output_file_new(const char *filename, int append)
{
    explain_output_t *result;

    result = explain_output_new(&vtable);
    if (result)
    {
        const char      *flags;
        explain_output_file_t *p;

        p = (explain_output_file_t *)result;
        flags = (append ? "wa" : "w");
        p->fp = fopen(filename, flags);
        if (!p->fp)
            p->fp = stderr;
#ifdef HAVE_SETLINEBUF
        setlinebuf(p->fp);
#endif
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
