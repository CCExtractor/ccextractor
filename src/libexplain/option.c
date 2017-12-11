/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2013 Peter Miller
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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/fstrcmp.h>
#include <libexplain/option.h>
#include <libexplain/output.h>
#include <libexplain/parse_bits.h>
#include <libexplain/program_name.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>
#include <libexplain/wrap_and_print.h>


typedef enum option_level_t option_level_t;
enum option_level_t
{
    option_level_default,
    option_level_something_or_die,
    option_level_environment_variable,
    option_level_client,
};

typedef int option_value_t;

typedef enum option_type_t option_type_t;
enum option_type_t
{
    option_type_bool,
    option_type_int
};

typedef struct option_t option_t;
struct option_t
{
    option_level_t level;
    option_value_t value;
    option_type_t type;
};

typedef char value_t;

static int initialised;
static option_t debug =
    { option_level_default, 0, option_type_bool };
static option_t numeric_errno =
    { option_level_default, 1, option_type_bool };
static option_t dialect_specific =
    { option_level_default, 1, option_type_bool };
static option_t assemble_program_name =
    { option_level_default, 1, option_type_bool };
static option_t symbolic_mode_bits =
    { option_level_default, 0, option_type_bool };
static option_t internal_strerror =
    { option_level_default, 0, option_type_bool };
static option_t hanging_indent =
    { option_level_default, 0, option_type_int };
static option_t extra_device_info =
    { option_level_default, 1, option_type_bool };

typedef struct table_t table_t;
struct table_t
{
    const char      *name;
    option_t        *location;
};

static const table_t table[] =
{
    { "assemble-program-name", &assemble_program_name },
    { "debug", &debug },
    { "dialect-specific", &dialect_specific },
    { "hanging-indent", &hanging_indent },
    { "internal-strerror", &internal_strerror },
    { "numeric-errno", &numeric_errno },
    { "program-name", &assemble_program_name },
    { "symbolic-mode-bits", &symbolic_mode_bits },
    { "extra-device-info", &extra_device_info },
};


static const explain_parse_bits_table_t bool_table[] =
{
    { "yes", 1 },
    { "no", 0 },
    { "true", 1 },
    { "false", 0 },
};


static void
process(char *name, option_level_t level)
{
    const table_t   *tp;
    value_t         value;
    char            *eq;
    int             invert;

    if (!*name)
        return;
    invert = 0;
    value = 1;
    if (name[0] == 'n' && name[1] == 'o' && name[2] == '-')
    {
        invert = 1;
        name += 3;
    }
    eq = strchr(name, '=');
    if (eq)
    {
        int n = 0;
        explain_parse_bits(eq + 1, bool_table, SIZEOF(bool_table), &n);
        value = n;
        while (eq > name && isspace((unsigned char)eq[-1]))
            --eq;
        *eq = '\0';
    }
    if (invert)
        value = !value;
    for (tp = table; tp < ENDOF(table); ++tp)
    {
        if (0 == strcmp(tp->name, name))
        {
            if (level >= tp->location->level)
            {
                tp->location->level = level;
                tp->location->value = value;
            }
            return;
        }
    }
    if (debug.value)
    {
        const table_t   *best_tp;
        double          best_weight;
        explain_string_buffer_t buf;
        char            message[200];

        best_tp = 0;
        best_weight = 0.6;
        for (tp = table; tp < ENDOF(table); ++tp)
        {
            double          weight;

            weight = explain_fstrcmp(tp->name, name);
            if (best_weight < weight)
            {
                best_tp = tp;
                best_weight = weight;
            }
        }

        explain_string_buffer_init(&buf, message, sizeof(message));
        explain_string_buffer_puts(&buf, "libexplain: Warning: option ");
        explain_string_buffer_puts_quoted(&buf, name);
        explain_string_buffer_puts(&buf, " unknown");
        if (best_tp)
        {
            if (level >= best_tp->location->level)
            {
                best_tp->location->level = level;
                best_tp->location->value = value;
            }

            explain_string_buffer_puts(&buf, ", assuming you meant ");
            explain_string_buffer_puts_quoted(&buf, best_tp->name);
            explain_string_buffer_puts(&buf, " instead");
        }
        explain_wrap_and_print(stderr, message);
    }
}


static void
initialise(void)
{
    const char      *cp;
    int             err;

    err = errno;
    initialised = 1;
    cp = getenv("EXPLAIN_OPTIONS");
    if (!cp)
    {
        /* for historical compatibility */
        cp = getenv("LIBEXPLAIN_OPTIONS");
    }
    if (!cp)
        cp = "";
    for (;;)
    {
        char            name[100];
        char            *np;
        unsigned char   c;

        c = *cp++;
        if (!c)
            break;
        if (isspace(c))
            continue;
        np = name;
        for (;;)
        {
            if (np < ENDOF(name) -1)
            {
                if (isupper(c))
                    c = tolower(c);
                *np++ = c;
            }
            c = *cp;
            if (!c)
                break;
            ++cp;
            if (c == ',')
                break;
        }
        /* remove trailing white space */
        while (np > name && isspace((unsigned char)np[-1]))
            --np;
        *np = '\0';

        process(name, option_level_environment_variable);
    }
    errno = err;
}


int
explain_option_debug(void)
{
    if (!initialised)
        initialise();
    return debug.value;
}


int
explain_option_numeric_errno(void)
{
    if (!initialised)
        initialise();
    return numeric_errno.value;
}


int
explain_option_dialect_specific(void)
{
    if (!initialised)
        initialise();
    return dialect_specific.value;
}


int
explain_option_assemble_program_name(void)
{
    if (!initialised)
        initialise();
    return assemble_program_name.value;
}


void
explain_program_name_assemble(int yesno)
{
    /*
     * This is the public interface, it has highest
     * precedence.  For the internal interface, see the
     * #explain_program_name_assemble_internal function.
     */
    if (!initialised)
        initialise();
    if (assemble_program_name.level <= option_level_client)
    {
        assemble_program_name.level = option_level_client;
        assemble_program_name.value = !!yesno;
    }
}


#if 0
void
explain_program_name_assemble_internal(int yesno)
{
    /*
     * This is the private interface.
     */
    if (!initialised)
        initialise();
    if (assemble_program_name.level <= option_level_something_or_die)
    {
        assemble_program_name.level = option_level_something_or_die;
        assemble_program_name.value = !!yesno;
    }
}
#endif


int
explain_option_symbolic_mode_bits(void)
{
    if (!initialised)
        initialise();
    return symbolic_mode_bits.value;
}


int
explain_option_internal_strerror(void)
{
    if (!initialised)
        initialise();
    return internal_strerror.value;
}


int
explain_option_hanging_indent(int width)
{
    int             max;
    int             n;

    if (!initialised)
        initialise();
    if (width <= 0 || width >= 65536)
        width = 80;
    max = (width + 5) / 10;
    n = hanging_indent.value;
    if (n <= 0)
        return 0;
    if (n > max)
        return max;
    return n;
}


void
explain_option_hanging_indent_set(int columns)
{
    if (!initialised)
        initialise();
    if (hanging_indent.level <= option_level_client)
    {
        if (columns < 0)
            columns = 0;
        hanging_indent.value = columns;
        hanging_indent.level = option_level_client;
    }
}


int
explain_option_extra_device_info(void)
{
    if (!initialised)
        initialise();
    return extra_device_info.value;
}


/* vim: set ts=8 sw=4 et : */
