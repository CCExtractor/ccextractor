/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/output.h>


typedef struct explain_output_tee_t explain_output_tee_t;
struct explain_output_tee_t
{
    explain_output_t inherited;
    explain_output_t *first;
    explain_output_t *second;
};


static void
tee_destructor(explain_output_t *op)
{
    explain_output_tee_t *p;

    p = (explain_output_tee_t *)op;
    explain_output_method_destructor(p->first);
    explain_output_method_destructor(p->second);
}


static void
tee_message(explain_output_t *op, const char *text)
{
    explain_output_tee_t *p;

    p = (explain_output_tee_t *)op;
    explain_output_method_message(p->first, text);
    explain_output_method_message(p->second, text);
}


static void
tee_exit(explain_output_t *op, int status)
{
    explain_output_tee_t *p;

    p = (explain_output_tee_t *)op;
    if (p->first->vtable->exit)
        p->first->vtable->exit(p->first, status);
    if (p->second->vtable->exit)
        p->second->vtable->exit(p->second, status);
}


static const explain_output_vtable_t vtable =
{
    tee_destructor,
    tee_message,
    tee_exit,
    sizeof(explain_output_tee_t)
};


explain_output_t *
explain_output_tee_new(explain_output_t *first, explain_output_t *second)
{
    explain_output_t *result;

    if (!first)
        return second;
    if (!second)
        return first;
    result = explain_output_new(&vtable);
    if (result)
    {
        explain_output_tee_t *p;

        p = (explain_output_tee_t *)result;
        p->first = first;
        p->second = second;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
