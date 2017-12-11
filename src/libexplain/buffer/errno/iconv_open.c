/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/iconv_open.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/fstrcmp.h>


static void
explain_buffer_errno_iconv_open_system_call(explain_string_buffer_t *sb, int
    errnum, const char *tocode, const char *fromcode)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "iconv_open(tocode = ");
    explain_buffer_pathname(sb, tocode);
    explain_string_buffer_puts(sb, ", fromcode = ");
    explain_buffer_pathname(sb, fromcode);
    explain_string_buffer_putc(sb, ')');
}


static char **known_names;
static size_t known_names_size;
static size_t known_names_allocated;


static char *
xstrdup(const char *text)
{
    char            *result;
    size_t          text_size;

    text_size = strlen(text);
    result = malloc(text_size + 1);
    if (!result)
        return (char *)text;
    memcpy(result, text, text_size + 1);
    return result;
}


static void
one_more_name(char *name)
{
    if (known_names_size >= known_names_allocated)
    {
        char            **new_known_names;
        size_t          new_known_names_allocated;
        size_t          j;

        new_known_names_allocated =
            known_names_allocated ? known_names_allocated * 2 : 19;
        while (known_names_size > new_known_names_allocated)
            new_known_names_allocated *= 2;
        assert(known_names_size < new_known_names_allocated);
        new_known_names =
            malloc(new_known_names_allocated * sizeof(char *));
        for (j = 0; j < known_names_size; ++j)
            new_known_names[j] = known_names[j];
        if (known_names)
            free(known_names);
        known_names_allocated = new_known_names_allocated;
        known_names = new_known_names;
        assert(known_names_size < known_names_allocated);
    }
    known_names[known_names_size++] = xstrdup(name);
}


static void
upcase_insitu(char *name)
{
    char            *ip;
    char            *op;

    /*
     * eglibc::iconv/iconv_open.c
     * Normalizes the name, by remove all characters beside alpha-numeric,
     * '_', '-', '/', '.', and ':'.
     */
    ip = name;
    op = name;
    for(;;)
    {
        unsigned char c = *ip++;
        if (!c)
        {
            *op = '\0';
            return;
        }
        if (islower(c))
            *op++ = toupper(c);
        else if (isalnum(c))
            *op++ = c;
        else if (strchr("_-/.:", c))
            *op++ = c;
        else
        {
            /* discard */
        }
    }
}


static char *
strdup_upcase(const char *text)
{
    const char      *ip;
    char            *op;
    char            *result;

    result = malloc(strlen(text) + 1);
    if (!result)
        return NULL;
    ip = text;
    op = result;
    for (;;)
    {
        unsigned char c = *ip++;
        if (!c)
        {
            *op = '\0';
            return result;
        }
        if (islower(c))
            *op++ = toupper(c);
        else if (isalnum(c))
            *op++ = c;
        else if (strchr("_-/.:", c))
            *op++ = c;
        else
        {
            /* discard */
        }
    }
    return result;
}


static void
get_list_of_known_names(void)
{
    FILE            *fp;

    /*
     * The only way to do this is via the iconv() command, which has
     * special eglibc support
     */
    fp = popen("iconv --list", "r");
    if (!fp)
        return;
    for (;;)
    {
        char            buffer[200];
        char            *slash;

        if (!fgets(buffer, sizeof(buffer), fp))
            break;
        slash = strpbrk(buffer, "/\r\n");
        if (slash)
            *slash = '\0';
        upcase_insitu(buffer);
        one_more_name(buffer);
    }
    pclose(fp);
}


static int
is_known_name(const char *name)
{
    size_t          j;

    for (j = 0; j < known_names_size; ++j)
    {
        char *known_name;

        known_name = known_names[j];
#ifdef HAV_STRVERSCMP
        if (0 == strverscmp(known_name, name))
            return 1;
#else
        if (0 == strcmp(known_name, name))
            return 1;
#endif
    }
    return 0;
}


static char *
known_names_fuzzy(const char *name)
{
    char            *best_name;
    double          best_weight;
    size_t          j;
    char            *name2;

    name2 = strdup_upcase(name);
    if (!name2)
        return NULL;
    get_list_of_known_names();
    best_name = NULL;
    best_weight = 0.6;
    for (j = 0; j < known_names_size; ++j)
    {
        char            *known_name;
        double          w;

        known_name = known_names[j];
        w = explain_fstrcmp(name2, known_name);
        if (w > best_weight)
        {
            best_name = known_name;
            best_weight = w;
        }
    }
    free(name2);
    return best_name;
}


static int
known_names_check(explain_string_buffer_t *sb, const char *locale,
    const char *locale_caption)
{
    char            *locale_fuzzy;

    if (!locale)
    {
        explain_buffer_is_the_null_pointer(sb, locale_caption);
        return 1;
    }

    get_list_of_known_names();
    if (is_known_name(locale))
    {
        /* no error here */
        return 0;
    }

    explain_string_buffer_printf
    (
        sb,
        /* FIXME: i18n */
        "the %s argument is not a known locale name",
        locale_caption
    );

    locale_fuzzy = known_names_fuzzy(locale);
    if (locale_fuzzy)
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        explain_string_buffer_printf
        (
            sb->footnotes,
            /* FIXME: i18n */
            "did you mean the \"%s\" locale instead?",
            locale_fuzzy
        );
    }
    return 1;
}


void
explain_buffer_errno_iconv_open_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, const char *tocode, const char *fromcode)
{
    (void)tocode;
    (void)fromcode;
    switch (errnum)
    {
    case EINVAL:
        if (known_names_check(sb, tocode, "tocode"))
            break;
        if (known_names_check(sb, fromcode, "fromcode"))
            break;

        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "The conversion from fromcode to tocode is not supported by "
            "the implementation"
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_iconv_open(explain_string_buffer_t *sb, int errnum, const
    char *tocode, const char *fromcode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_iconv_open_system_call(&exp.system_call_sb, errnum,
        tocode, fromcode);
    explain_buffer_errno_iconv_open_explanation(&exp.explanation_sb, errnum,
        "iconv_open", tocode, fromcode);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
