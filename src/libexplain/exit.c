/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
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

#include <libexplain/exit.h>
#include <libexplain/option.h>
#include <libexplain/program_name.h>
#include <libexplain/string_buffer.h>
#include <libexplain/wrap_and_print.h>


/*
 * All ANSI C compliant system provide atexit,
 * but on_exit is a GNU glibc invention,
 * and a very useful one.
 */
#ifdef HAVE_ON_EXIT

enum action_t
{
    action_never,
    action_error,
    action_always,
};

static int      registered;
static enum action_t action = action_never;


static void
grim_reaper(int es, void *aux)
{
    es &= 255;
    (void)aux;
    switch (action)
    {
    default:
    case action_never:
        break;

    case action_error:
        if (es == EXIT_SUCCESS)
            break;
        /* fall through... */

    case action_always:
        {
            explain_string_buffer_t sb;
            char            text[1000];

            explain_string_buffer_init(&sb, text, sizeof(text));

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

            explain_string_buffer_puts(&sb, "exit status ");
            switch (es)
            {
            case EXIT_SUCCESS:
                explain_string_buffer_puts(&sb, "EXIT_SUCCESS");
                break;

            case EXIT_FAILURE:
                explain_string_buffer_puts(&sb, "EXIT_FAILURE");
                break;

            default:
                explain_string_buffer_printf(&sb, "EXIT_FAILURE (%d)", es);
                break;
            }

            explain_wrap_and_print(stderr, text);
        }
        break;
    }
}


static void
install_grim_reaper(void)
{
    if (!registered)
    {
        registered = 1;
        on_exit(grim_reaper, 0);
    }
}

#endif

void
explain_exit_on_exit(void)
{
#ifdef HAVE_ON_EXIT
    action = action_always;
    install_grim_reaper();
#endif
}


void
explain_exit_on_error(void)
{
#ifdef HAVE_ON_EXIT
    action = action_error;
    install_grim_reaper();
#endif
}


void
explain_exit_cancel(void)
{
#ifdef HAVE_ON_EXIT
    /*
     * If grim reaper is not already installed, there is no need to do
     * it now, because the result of installing it would be the same as
     * not installing it.
     */
    action = action_never;
#endif
}


/* vim: set ts=8 sw=4 et : */
