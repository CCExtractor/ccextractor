/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

%{

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/grp.h>
#include <libexplain/ac/pwd.h>
#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/acl.h>

#include <libexplain/acl_grammar.h>
#include <libexplain/gcc_attributes.h>

#define YYERROR_VERBOSE 1
#define YYDEBUG 0

/* is this forward enough? */
static int yyparse(void);

%}

%token COLON
%token COMMA
%token GROUP
%token JUNK
%token MASK
%token NAME
%token NUMBER
%token OTHER
%token PERMS
%token TAG
%token USER

%union
{
    acl_perm_t lv_perms;
    acl_tag_t lv_tag;
    long lv_number;
    const char *lv_name;
}

%type <lv_tag> TAG
%type <lv_number> NUMBER
%type <lv_number> user_qualifier
%type <lv_number> group_qualifier
%type <lv_perms> PERMS
%type <lv_perms> perms
%type <lv_name> NAME


%{

#if YYDEBUG
extern int yydebug;
#endif

static const char *lex_text_0;
static const char *lex_text_1;
static const char *lex_text;
static char *result;
static size_t result_used;
static size_t result_allocated;
static int yyparse(void);

/*
 * This gets called when we already know the string is unparse-able.
 * So the idea is to accumulate an explanation.
 */

char *
explain_acl_parse(const char *text)
{
#if YYDEBUG
    yydebug = 1;
#endif
    lex_text = text;
    lex_text_0 = text;
    result_used = 0;
    yyparse();
    return result;
}


static void
result_append(const char *text)
{
    size_t text_size = strlen(text);
    if (result_used + text_size + 3 > result_allocated)
    {
        char *new_result;
        size_t new_result_allocated =
            result_allocated ? 2 * result_allocated : 107;
        while (result_used + text_size + 3 > new_result_allocated)
            new_result_allocated *= 2; /* for O(1) behavour */
        new_result = malloc(new_result_allocated);
        memcpy(new_result, result, result_used);
        free(result);
        result = new_result;
        result_allocated = new_result_allocated;
    }
    memcpy(result + result_used, text, text_size);
    result_used += text_size;
    result[result_used] = '\0';
}


static void
yyerror(const char *text)
{
    if (result_used > 0)
        result_append(", ");

    /* indicate the string position */
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "[%d] ", (int)(lex_text_1 - lex_text_0));
        result_append(buf);
    }

    result_append(text);
}


static void yyerrorf(const char *fmt, ...) LIBEXPLAIN_FORMAT_PRINTF(1, 2);

static void
yyerrorf(const char *fmt, ...)
{
    va_list ap;
    char buf[200];

    va_start(ap, fmt);;
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    yyerror(buf);
}


static int expect_perms_next;
YYSTYPE yylval;

static int
looks_like_permissions(const char *text)
{
    acl_perm_t perms = 0;
    const char *cp = text;
    for (;;)
    {
        unsigned char c = *cp++;
        switch (c)
        {
        case '\0':
            --cp;
            yylval.lv_perms = perms;
            if ((cp - text) < 3)
            {
                yyerrorf("too few permissions bits (%s)", text);
                return 1;
            }
            if ((cp - text) > 3)
            {
                yyerrorf("too many permissions bits (%s)", text);
                return 1;
            }
            return 1;

        case 'R':
        case 'r':
            if ((cp - text) != 1)
                yyerrorf("the read permission must be first (%s)", text);
            perms |= ACL_READ;
            break;

        case 'W':
        case 'w':
            if ((cp - text) != 2)
                yyerrorf("the write permission must be second (%s)", text);
            perms |= ACL_WRITE;
            break;

        case 'X':
        case 'x':
            if ((cp - text) != 3)
            {
                yyerrorf
                (
                    "the execute or search permission must be third (%s)",
                    text
                );
            }
            perms |= ACL_EXECUTE;
            break;

        case '-':
            break;

        default:
            if ((cp - text) <= 3 && isprint(c))
            {
                yyerrorf("permission bit '%c' unknown (%s)", c, text);
                break;
            }
            return 0;
        }
    }
}


static int
check_for_keywords(const char *text)
{
    if (!strcmp(text, "u") || !strcmp(text, "user"))
    {
        yylval.lv_tag = ACL_USER;
        return USER;
    }
    if (!strcmp(text, "g") || !strcmp(text, "group"))
    {
        yylval.lv_tag = ACL_GROUP;
        return GROUP;
    }
    if (!strcmp(text, "o") || !strcmp(text, "other"))
    {
        yylval.lv_tag = ACL_OTHER;
        return OTHER;
    }
    if (!strcmp(text, "m") || !strcmp(text, "mask"))
    {
        yylval.lv_tag = ACL_MASK;
        return MASK;
    }
    return 0;
}


static int
yylex(void)
{
    for (;;)
    {
        unsigned char c;

        lex_text_1 = lex_text;
        c = *lex_text++;
        switch (c)
        {
        case '\0':
            --lex_text;
            return 0;

        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
            continue;

        case '#':
            /* comments */
            for (;;)
            {
                c = *lex_text++;
                if (!c)
                {
                    --lex_text;
                    break;
                }
                if (c == '\n')
                    break;
            }
            continue;

        case ':':
            return COLON;

        case ',':
            return COMMA;

        default:
            return JUNK;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                long n = 0;
                for (;;)
                {
                    n = n * 10 + c - '0';
                    c = *lex_text++;
                    switch (c)
                    {
                    case '\0':
                        --lex_text;
                        break;

                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        continue;

                    default:
                        --lex_text;
                        break;
                    }
                    break;
                }
                yylval.lv_number = n;
            }
            return NUMBER;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z':
        case '-': case '_': case '.':
            {
                static char name[100];
                char *np = name;
                for (;;)
                {
                    if (np + 2 < name + sizeof(name))
                        *np++ = c;
                    c = *lex_text++;
                    switch (c)
                    {
                    case '\0':
                        --lex_text;
                        break;

                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
                    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
                    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
                    case 'y': case 'z':
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
                    case 'Y': case 'Z':
                    case '-': case '_': case '.':
                        continue;

                    default:
                        --lex_text;
                        break;
                    }
                    break;
                }
                *np = '\0';
                yylval.lv_name = name;

                if (expect_perms_next)
                {
                    if (looks_like_permissions(name))
                    {
                        return PERMS;
                    }
                }

                {
                    int kw = check_for_keywords(name);
                    if (kw)
                    {
                        return kw;
                    }
                }
            }
            return NAME;
        }
    }
}


static int
check_user_name(const char *name)
{
    struct passwd *pw;

    pw = getpwnam(name);
    if (!pw)
    {
        yyerrorf("user \"%s\" unknown", name);
        return -1;
    }
    return pw->pw_uid;
}


static int
check_user_id(int uid)
{
    struct passwd *pw;

    pw = getpwuid(uid);
    if (!pw)
    {
        yyerrorf("user %d unknown", uid);
        return -1;
    }
    return pw->pw_uid;
}


static int
check_group_name(const char *name)
{
    struct group *gr;

    gr = getgrnam(name);
    if (!gr)
    {
        yyerrorf("group \"%s\" unknown", name);
        return -1;
    }
    return gr->gr_gid;
}


static int
check_group_id(int gid)
{
    struct group *gr;

    gr = getgrgid(gid);
    if (!gr)
    {
        yyerrorf("group %d unknown", gid);
        return -1;
    }
    return gr->gr_gid;
}


%}


%%

acl
    : entries
    | /* empty */
        {
            yyerror
            (
                "there must be at least one entry in an Access Control List"
            );
        }
    ;

entries
    : entry
    | entries entry
    | entries COMMA entry
    ;

entry
    : USER COLON user_qualifier COLON perms
    | USER COLON COLON perms
    | GROUP COLON group_qualifier COLON perms
    | GROUP COLON COLON perms
    | MASK COLON COLON perms
    | OTHER COLON COLON perms
    | bogus_tag COLON bogus_qualifier COLON perms
    | error
    ;

bogus_tag
    : NAME
        {
            yyerrorf("acl tag \"%s\" unknown", $1);
        }
    ;

bogus_qualifier
    : /* empty */
    | NAME
    | NUMBER
    ;

user_qualifier
    : NAME
        {
            $$ = check_user_name($1);
        }
    | NUMBER
        {
            $$ = check_user_id($1);
        }
    ;

group_qualifier
    : NAME
        {
            $$ = check_group_name($1);
        }
    | NUMBER
        {
            $$ = check_group_id($1);
        }
    ;

perms
    : perms_flag PERMS perms_unflag
        { $$ = $2; }
    | perms_flag error perms_unflag
        { $$ = 0; }
    ;

perms_flag
    : { expect_perms_next = 1; }
    ;

perms_unflag
    : { expect_perms_next = 0; }
    ;

/* vim: set ts=8 sw=4 et : */
