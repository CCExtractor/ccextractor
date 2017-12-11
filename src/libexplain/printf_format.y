/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

%{

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/printf_format.h>

/* #define YYDEBUG 1 */

%}

%token CHAR
%token DIG19
%token FLAG
%token MOD
%token POSITION
%token SPEC

%union
{
    int             lv_int;
}

%type <lv_int> CHAR
%type <lv_int> DIG19
%type <lv_int> FLAG
%type <lv_int> MOD
%type <lv_int> POSITION
%type <lv_int> SPEC
%type <lv_int> digit
%type <lv_int> flag
%type <lv_int> flags
%type <lv_int> modifier
%type <lv_int> number
%type <lv_int> number0
%type <lv_int> position
%type <lv_int> precision
%type <lv_int> width

%{

enum parse_mode_t
{
    parse_mode_text,
    parse_mode_spec
};
typedef enum parse_mode_t parse_mode_t;

static parse_mode_t parse_mode;
static const char *text;
static int      number_of_errors;
static int      arg_number;
static explain_printf_format_list_t *results;
int yyparse(void);
#if YYDEBUG
extern int yydebug;
#endif


size_t
explain_printf_format(const char *texta, explain_printf_format_list_t *rslt)
{
    int             ret;

    parse_mode = parse_mode_text;
    text = texta;
    number_of_errors = 0;
    arg_number = 0;
    results = rslt;

#if YYDEBUG
    yydebug = 1;
#endif
    explain_printf_format_list_clear(results);
    ret = yyparse();
    if (ret != 0 || number_of_errors > 0)
    {
        size_t          nbytes;

        explain_printf_format_list_clear(results);
        /*
         * We return a number to indicate where in the string the
         * problem occurs.  This allows the offending sequence to be
         * inserted into the error message.
         *
         * Note: this will always the positive, because you have to
         * parse *something* to get an error.
         */
        nbytes = text - texta;
        assert(nbytes > 0);
        return nbytes;
    }
    return 0;
}

static int yylex(void); /* forward */
static void yyerror(const char *text); /* forward */

%}


%%

format_string
    : /* empty */
    | format_string CHAR
    | format_string format
        { parse_mode = parse_mode_text; }
    ;

position
    : /* empty */
        { $$ = arg_number++; }
    | POSITION
        { $$ = $1; }
    ;

format
    : '%' position flags width precision modifier SPEC
        {
            explain_printf_format_t fmt;

            fmt.index = $2;
            fmt.flags = $3;
            fmt.width = $4;
            fmt.precision = $5;
            fmt.modifier = $6;
            fmt.specifier = $7;
            if (fmt.specifier == 'm')
                fmt.index = -1;
            switch (fmt.specifier)
            {
            case 'C':
                fmt.modifier = 'l';
                fmt.specifier = 'c';
                break;

            case 'D':
                /* FIXME: obsolete, generate an error */
                fmt.modifier = 'l';
                fmt.specifier = 'd';
                break;

            case 'O':
                /* FIXME: obsolete, generate an error */
                fmt.modifier = 'l';
                fmt.specifier = 'o';
                break;

            case 'S':
                fmt.modifier = 'l';
                fmt.specifier = 's';
                break;

            case 'U':
                /* FIXME: obsolete, generate an error */
                fmt.modifier = 'l';
                fmt.specifier = 'u';
                break;

            default:
                break;
            }
            explain_printf_format_list_append(results, &fmt);
        }
    | '%' '%'
    ;

flags
    : /* empty */
        { $$ = 0; }
    | flags flag
        { $$ = $1 | $2; }
    ;

flag
    : FLAG
        { $$ = $1; }
    | '0'
        { $$ = FLAG_ZERO; }
    ;

width
    : /*empty */
        { $$ = -1; }
    | number
        { $$ = $1; }
    | '*' position
        {
            explain_printf_format_t fmt;

            $$ = -1;

            fmt.index = $2;
            fmt.flags = 0;
            fmt.width = 0;
            fmt.precision = 0;
            fmt.modifier = 0;
            fmt.specifier = '*';
            explain_printf_format_list_append(results, &fmt);
        }
    ;

precision
    : /* empty */
        { $$ = -1; }
    | '.' number0
        { $$ = $2; }
    | '.' '*' position
        {
            explain_printf_format_t fmt;

            $$ = -1;

            fmt.index = $3;
            fmt.flags = 0;
            fmt.width = 0;
            fmt.precision = 0;
            fmt.modifier = 0;
            fmt.specifier = '*';
            explain_printf_format_list_append(results, &fmt);
        }
    ;

number
    : DIG19
        { $$ = $1; }
    | number digit
        { $$ = $1 * 10 + $2; }
    ;

number0
    : digit
        { $$ = $1; }
    | number0 digit
        { $$ = $1 * 10 + $2; }
    ;

digit
    : '0'
        { $$ = 0; }
    | DIG19
        { $$ = $1; }
    ;

modifier
    : /* empty */
        { $$ = 0; }
    | MOD
        { $$ = $1; }
    | 'h'
        { $$ = 'h'; }
    | 'h' 'h'
        { $$ = 'H'; }
    | 'l'
        { $$ = 'l'; }
    | 'l' 'l'
        { $$ = 'L'; }
    ;

%%


static int
yylex(void)
{
    unsigned char   c;

    for (;;)
    {
        c = *text;
        if (c == '\0')
            return 0;
        ++text;
        if (parse_mode != parse_mode_text)
            break;
        if (c == '%')
        {
            parse_mode = parse_mode_spec;
            return c;
        }
    }
    switch (c)
    {
    case 'A':
    case 'C':
    case 'D': /* obsolete */
    case 'E':
    case 'F': /* synonym for 'f' */
    case 'G':
    case 'O': /* obsolete */
    case 'S':
    case 'U': /* obsolete */
    case 'X':
    case 'a':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'i':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 's':
    case 'u':
    case 'x':
        yylval.lv_int = c;
        return SPEC;

    case 'L':
    case 'j':
    case 'q':
    case 't':
    case 'z':
        yylval.lv_int = c;
        return MOD;

    case 'Z':
        yylval.lv_int = 'z';
        return MOD;

    case ' ':
        yylval.lv_int = FLAG_SPACE;
        return FLAG;

    case '#':
        yylval.lv_int = FLAG_HASH;
        return FLAG;

    case '\'':
        yylval.lv_int = FLAG_THOUSANDS;
        return FLAG;

    case '+':
        yylval.lv_int = FLAG_PLUS;
        return FLAG;

    case '-':
        yylval.lv_int = FLAG_MINUS;
        return FLAG;

    case 'I':
        yylval.lv_int = FLAG_I18N;
        return FLAG;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        {
            /* it could be a position */
            const char *cp = text;
            int n = c - '0';
            while (isdigit(*cp))
                n = n * 10 + *cp++ - '0';
            if (*cp == '$')
            {
                text = cp + 1;
                yylval.lv_int = n - 1;
                return POSITION;
            }
            yylval.lv_int = c - '0';
            return DIG19;
        }

    default:
        break;
    }
    return c;
}

#include <libexplain/string_buffer.h>

static void
yyerror(const char *s)
{
    (void)s;
#if YYDEBUG
    if (yydebug)
    {
        char            q[1000];
        explain_string_buffer_t sb;

        explain_string_buffer_init(&sb, q, sizeof(q));
        explain_string_buffer_puts_quoted(&sb, s);
        fprintf(stderr, "%s: %d: %s\n", __FILE__, __LINE__, q);
    }
#endif
    ++number_of_errors;
}


/* vim: set ts=8 sw=4 et : */
