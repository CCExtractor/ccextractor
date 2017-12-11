/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/termios.h>
#include <libexplain/ac/unistd.h>
#include <libexplain/ac/wchar.h>
#include <libexplain/ac/wctype.h>

#ifdef HAVE_winsize_SYS_IOCTL_H
#include <libexplain/ac/sys/ioctl.h>
#endif

#include <libexplain/option.h>
#include <libexplain/string_buffer.h>
#include <libexplain/wrap_and_print.h>

#define DEFAULT_LINE_WIDTH 75

#define MAX_LINE_LENGTH (PATH_MAX + 10)


#if defined(HAVE_MBRTOWC) && defined(HAVE_WCWIDTH)

void
explain_wrap_and_print_width(FILE *fp, const char *text, int width)
{
    const char      *cp;
    const char      *end;
    char            line_string[MAX_LINE_LENGTH + 1];
    explain_string_buffer_t line_buf;
    char            word_string[MAX_LINE_LENGTH + 1];
    explain_string_buffer_t word_buf;
    static mbstate_t mbz;
    mbstate_t       state;
    int             width_of_line;
    int             width_of_word;
    int             hanging_indent;
    int             first_line;

    assert(width > 0);
    if (width <= 0)
        width = DEFAULT_LINE_WIDTH ;
    if (width > MAX_LINE_LENGTH)
        width = MAX_LINE_LENGTH;
    hanging_indent = explain_option_hanging_indent(width);
    assert(sizeof(word_string) <= sizeof(line_string));
    cp = text;
    end = text + strlen(text);
    explain_string_buffer_init(&line_buf, line_string, sizeof(line_string));
    explain_string_buffer_init(&word_buf, word_string, sizeof(word_string));
    state = mbz;
    width_of_line = 0;
    width_of_word = 0;
    first_line = 1;
    for (;;)
    {
        const char      *starts_here;
        wchar_t         wc;
        size_t          n;

        starts_here = cp;
        n = mbrtowc(&wc, cp, end - cp, &state);
        if ((ssize_t)n < 0)
        {
            wc = *cp;
            n = 1;
        }
        cp += n;
        if (n == 0 || wc == L'\0')
        {
            if (line_buf.position)
            {
                if (0 == fwrite(line_string, line_buf.position, 1, fp))
                    return;
                putc('\n', fp);
            }
            return;
        }

        if (iswspace(wc))
            continue;

        /*
         * Grab the next word.
         *
         * The width_of_word records the number of character positions
         * consumed.  This can be less than the number of bytes, when
         * multi-byte character sequences represent single displayed
         * characters.  This can be more than the number of bytes, for
         * exmaple kanji, when a character is display 2 columns wide.
         */
        word_buf.position = 0;
        width_of_word = 0;
        for (;;)
        {
            mbstate_t       hold;

            explain_string_buffer_write(&word_buf, starts_here, n);
            width_of_word += wcwidth(wc);
            if (explain_string_buffer_full(&word_buf))
                break;

            hold = state;
            starts_here = cp;
            n = mbrtowc(&wc, cp, end - cp, &state);
            if ((ssize_t)n < 0)
            {
                wc = *cp;
                n = 1;
            }

            if (n == 0 || wc == '\0')
            {
                state = hold;
                break;
            }
            if (iswspace(wc))
            {
                state = hold;
                break;
            }
            cp += n;
        }

        if (line_buf.position == 0)
        {
            /* do nothing */
        }
        else if
        (
            width_of_line + 1 + width_of_word
        <=
            width - (first_line ? 0 : hanging_indent))
        {
            explain_string_buffer_putc(&line_buf, ' ');
            ++width_of_line;
        }
        else
        {
            if (0 == fwrite(line_string, line_buf.position, 1, fp))
                return;
            putc('\n', fp);
            line_buf.position = 0;
            width_of_line = 0;
            first_line = 0;
        }
        if (line_buf.position == 0 && !first_line && hanging_indent)
        {
            for (;;)
            {
                explain_string_buffer_putc(&line_buf, ' ');
                if (line_buf.position >= (size_t)hanging_indent)
                    break;
            }
        }
        explain_string_buffer_puts(&line_buf, word_string);
        width_of_line += width_of_word;

        /*
         * Note: it is possible for a line to be longer than (width)
         * when it contains a single word that is itself longer than
         * (width).  We do this to avoid putting line breaks in the
         * middle of pathnames, provided the pathanme itself does not
         * contain white space.  This is useful for copy-and-paste.
         */
    }
}

#else

void
explain_wrap_and_print_width(FILE *fp, const char *text, int width)
{
    const char      *cp;
    char            line_string[MAX_LINE_LENGTH + 1];
    explain_string_buffer_t line_buf;
    char            word_string[MAX_LINE_LENGTH + 1];
    explain_string_buffer_t word_buf;
    unsigned        hanging_indent;
    int             first_line;

    assert(width > 0);
    if (width <= 0)
        width = DEFAULT_LINE_WIDTH ;
    if (width > MAX_LINE_LENGTH)
        width = MAX_LINE_LENGTH;
    hanging_indent = explain_option_hanging_indent(width);
    assert(sizeof(word_string) <= sizeof(line_string));
    cp = text;
    explain_string_buffer_init(&line_buf, line_string, sizeof(line_string));
    explain_string_buffer_init(&word_buf, word_string, sizeof(word_string));
    first_line = 1;
    for (;;)
    {
        unsigned char c = *cp++;
        if (c == '\0')
        {
            if (line_buf.position)
            {
                if (0 == fwrite(line_string, line_buf.position, 1, fp))
                    return;
                putc('\n', fp);
            }
            return;
        }

        if (isspace(c))
            continue;

        /*
         * Grab the next word.
         */
        word_buf.position = 0;
        for (;;)
        {
            explain_string_buffer_putc(&word_buf, c);
            if (explain_string_buffer_full(&word_buf))
                break;
            c = *cp;
            if (c == '\0')
                break;
            if (isspace(c))
                break;
            ++cp;
        }

        if (line_buf.position == 0)
        {
            /* do nothing */
        }
        else if
        (
            line_buf.position + 1 + word_buf.position
        <=
            (size_t)(width - (first_line ? 0 : hanging_indent))
        )
        {
            explain_string_buffer_putc(&line_buf, ' ');
        }
        else
        {
            if (0 == fwrite(line_string, line_buf.position, 1, fp))
                return;
            putc('\n', fp);
            line_buf.position = 0;
            first_line = 0;
        }
        if (line_buf.position == 0 && !first_line && hanging_indent)
        {
            for (;;)
            {
                explain_string_buffer_putc(&line_buf, ' ');
                if (line_buf.position >= hanging_indent)
                    break;
            }
        }
        explain_string_buffer_puts(&line_buf, word_string);
        /*
         * Note: it is possible for a line to be longer than (width)
         * when it contains a single word that is itself longer than
         * (width).
         */
    }
}

#endif


void
explain_wrap_and_print(FILE *fp, const char *text)
{
    int             width;
    int             fildes;

    width = DEFAULT_LINE_WIDTH;

    /*
     * If output is going to a terminal, use the terminal's with when
     * formatting error messages.
     */
    fildes = fileno(fp);
    if (isatty(fildes))
    {
        const char      *cp;

        cp = getenv("COLS");
        if (cp && *cp)
        {
            char            *ep;

            width = strtol(cp, &ep, 0);
            if (ep == cp || *ep)
                width = 0;
        }
#ifdef TIOCGWINSZ
        if (width <= 0 )
        {
            struct winsize  ws;

            if (ioctl(fildes, TIOCGWINSZ, &ws) >= 0)
                width = ws.ws_col;
        }
#endif
        if (width <= 0)
            width = DEFAULT_LINE_WIDTH;
    }

    /*
     * Print the text using the window width.
     */
    explain_wrap_and_print_width(fp, text, width);
}


/* vim: set ts=8 sw=4 et : */
