/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/errno/execlp.h>
#include <libexplain/buffer/errno/execvp.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


static void
explain_buffer_errno_execlp_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int argc, const char **argv)
{
    size_t          argsize;
    int             n;

    (void)errnum;
    explain_string_buffer_puts(sb, "execlp(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", arg = ");

    /*
     * produce output similar to execve
     */
    argsize = 0;
    for (n = 0; n < argc && argsize < 1000; ++n)
    {
        const char      *s;

        if (n)
            explain_string_buffer_puts(sb, ", ");
        /*
         * We know argv is not EFAULT, because we made it, but we don't
         * know about argv[n].
         */
        s = argv[n];
        if (explain_is_efault_path(s))
        {
            explain_buffer_pointer(sb, s);
            argsize += 8;
        }
        else
        {
            explain_string_buffer_puts_quoted(sb, s);
            argsize += strlen(s);
        }
    }
    if (n < argc)
    {
        explain_string_buffer_printf
        (
            sb,
            /* FIXME: i18n */
            "... plus another %d command line arguments",
            (int)(argc - n)
        );
    }
    explain_string_buffer_puts(sb, ", NULL)");
}


void
explain_buffer_errno_execlp_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *pathname, int argc, const char **argv)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/execlp.html
     */
    (void)argc;
    explain_buffer_errno_execvp_explanation
    (
        sb,
        errnum,
        syscall_name,
        pathname,
        (char *const *)argv
    );
}


void
explain_buffer_errno_execlpv(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int argc, const char **argv)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_execlp_system_call(&exp.system_call_sb, errnum,
        pathname, argc, argv);
    explain_buffer_errno_execlp_explanation(&exp.explanation_sb, errnum,
        "execlp", pathname, argc, argv);
    explain_explanation_assemble(&exp, sb);
}


void
explain_buffer_errno_execlp(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const char *arg, va_list ap)
{
    va_list         ap1;
    va_list         ap2;
    int             argc;
    int             argc2;
    const char      **argv;
    const char      *dummy[100];

    /*
     * count the number of arguments
     */
    va_copy(ap1, ap);
    argc = 1;
    for (;;)
    {
        const char *p = va_arg(ap1, const char *);
        if (!p)
            break;
        ++argc;
    }

    /*
     * create an array to store the arguments in.
     */
    if (argc < (int)SIZEOF(dummy))
    {
        argv = dummy;
    }
    else
    {
        argv = malloc((argc + 1) * sizeof(const char *));
        if (!argv)
        {
            argv = dummy;
            argc = SIZEOF(dummy) - 1;
        }
    }

    /*
     * Now slurp the arguments into the array.
     */
    va_copy(ap2, ap);
    argv[0] = arg;
    for (argc2 = 1; argc2 < argc; ++argc2)
    {
        argv[argc2] = va_arg(ap2, const char *);
    }
    argv[argc2] = NULL;

    /* for ANSI C compliance */
    va_end(ap1);

    explain_buffer_errno_execlpv(sb, errnum, pathname, argc, argv);

    if (argv != dummy)
        free(argv);
}


/* vim: set ts=8 sw=4 et : */
