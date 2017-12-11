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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdlib.h>

#include <libexplain/output.h>
#include <libexplain/output/stderr.h>


explain_output_t *
explain_output_new(const explain_output_vtable_t *vtable)
{
    unsigned int    size;
    explain_output_t *result;

    assert(vtable);
    size = vtable->size;
    if (size < sizeof(explain_output_t))
        size = sizeof(explain_output_t);
    result = malloc(size);
    if (!result)
        return NULL;
    result->vtable = vtable;
    result->exit_has_been_used = 0;
    return result;
}


void
explain_output_method_destructor(explain_output_t *op)
{
    if (!op)
        return;
    if (op == &explain_output_static_stderr)
        return;
    assert(op->vtable);
    if (op->vtable->destructor)
        op->vtable->destructor(op);
    free(op);
}


void
explain_output_method_message(explain_output_t *op, const char *text)
{
    int             hold_errno;

    hold_errno = errno;
    assert(op);
    assert(op->vtable);
    assert(op->vtable->message);
    assert(text);
    op->vtable->message(op, text);
    errno = hold_errno;
}


void _exit(int);


void
explain_output_method_exit(explain_output_t *op, int status)
{
    assert(op);
    assert(op->vtable);
    if (op->vtable->exit)
        op->vtable->exit(op, status);

    /**
      * POSIX and the C standard both say that it should not call
      * 'exit', because the behavior is undefined if 'exit' is called
      * more than once.  So we call '_exit' instead of 'exit'.
      */
    if (op->exit_has_been_used)
        _exit(status);

    /*
     * 1. We call ::exit if they didn't supply an exit method.
     * 2. We ::exit if their exit method returns, because the rest of
     *    the libexplain code assumes that exit means ::exit.
     */
    exit(status);
    op->exit_has_been_used = 1;
}


/* vim: set ts=8 sw=4 et : */
