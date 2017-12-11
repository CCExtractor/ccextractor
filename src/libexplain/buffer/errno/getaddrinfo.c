/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/addrinfo.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/getaddrinfo.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errcode_getaddrinfo_system_call(
    explain_string_buffer_t *sb, int errcode, const char *node,
    const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    (void)errcode;
    explain_string_buffer_puts(sb, "getaddrinfo(node = ");
    explain_buffer_pathname(sb, node);
    explain_string_buffer_puts(sb, ", service = ");
    explain_buffer_pathname(sb, service);
    explain_string_buffer_puts(sb, ", hints = ");
    explain_buffer_addrinfo(sb, hints);
    explain_string_buffer_puts(sb, ", res = ");
    explain_buffer_pointer(sb, res);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errcode_getaddrinfo_explanation(
    explain_string_buffer_t *sb, int errcode, const char *node,
    const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    if (errcode > 0)
    {
        switch (errcode)
        {
        case EFAULT:
            if (node && explain_is_efault_string(node))
            {
                explain_buffer_efault(sb, "node");
                break;
            }
            if (service && explain_is_efault_string(service))
            {
                explain_buffer_efault(sb, "service");
                break;
            }
            if (hints && explain_is_efault_pointer(hints, sizeof(*hints)))
            {
                explain_buffer_efault(sb, "hints");
                break;
            }
            if (res && explain_is_efault_pointer(res, sizeof(*res)))
            {
                explain_buffer_efault(sb, "res");
                break;
            }
            break;

        case ENOMEM:
            explain_buffer_enomem_kernel(sb);
            break;

        default:
            break;
        }
        return;
    }

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getaddrinfo.html
     */
    switch (errcode)
    {
#ifdef EAI_ADDRFAMILY
    case EAI_ADDRFAMILY:
        explain_string_buffer_puts
        (
            sb,
            "the specified network host does not have any network "
            "addresses in the requested address family"
        );
#endif

    case EAI_AGAIN:
        explain_string_buffer_puts
        (
            sb,
            "the name server returned a temporary failure indication, "
            "future attempts may succeed"
        );
        break;

    case EAI_BADFLAGS:
        explain_string_buffer_puts
        (
            sb,
            "the ai->ai_flags parameter had an invalid value"
        );
        break;

    case EAI_FAIL:
        explain_string_buffer_puts
        (
            sb,
            "the name server returned a permanent failure indication "
            "when attempting to resolve the name"
        );
        break;

    case EAI_FAMILY:
        explain_string_buffer_puts
        (
            sb,
            "the requested address family is not supported"
        );
        break;

#ifdef EAI_NODATA
    case EAI_NODATA:
        explain_string_buffer_puts
        (
            sb,
            "the specified network host exists, but does not have any "
            "network addresses defined"
        );
        break;
#endif

    case EAI_NONAME:
        if (!node && !service)
        {
            explain_string_buffer_puts
            (
                sb,
                "both node and service are NULL, you have to specify at "
                "least one of them"
            );
            break;
        }
#ifdef AI_NUMERICSERV
        if (hints && (hints->ai_flags & AI_NUMERICSERV) && service)
        {
            explain_string_buffer_puts
            (
                sb,
                "service was not a numeric port-number string"
            );
            break;
        }
#endif
        if (!service)
        {
            explain_string_buffer_puts(sb, "the node is not known");
            break;
        }
        if (!node)
        {
            explain_string_buffer_puts(sb, "the service is not known");
            break;
        }
        explain_string_buffer_puts
        (
            sb,
            "the node or service is not known"
        );
        break;

    case EAI_SERVICE:
        explain_string_buffer_puts
        (
            sb,
            "the requested service is not available for the requested "
            "socket type, it may be available through another socket "
            "type"
        );
        break;

    case EAI_SOCKTYPE:
        explain_string_buffer_puts
        (
            sb,
            "the requested socket type is not supported"
        );
        break;

    case EAI_SYSTEM:
        explain_string_buffer_puts
        (
            sb,
            "a system error occurred, the error code can be found in errno"
        );
        break;

    case EAI_MEMORY:
        explain_buffer_enomem_user(sb, 0);
        break;

#ifdef EAI_OVERFLOW
    case EAI_OVERFLOW:
        explain_string_buffer_puts
        (
            sb,
            "an argument buffer overflowed"
        );
        break;
#endif

    default:
        break;
    }
}


void
explain_buffer_errcode_getaddrinfo(explain_string_buffer_t *sb,
    int errcode, const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errcode);
    explain_buffer_errcode_getaddrinfo_system_call
    (
        &exp.system_call_sb,
        errcode,
        node,
        service,
        hints,
        res
    );
    explain_buffer_errcode_getaddrinfo_explanation
    (
        &exp.explanation_sb,
        errcode,
        node,
        service,
        hints,
        res
    );
    explain_explanation_assemble_gai(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
