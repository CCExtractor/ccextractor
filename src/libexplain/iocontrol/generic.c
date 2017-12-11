/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013, 2014 Peter Miller
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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdint.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/ebusy.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/enomedium.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int8.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/short.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


#if defined(_IOC_DIR) || defined(IOC_DIRMASK)

static const explain_parse_bits_table_t ioc_dir_table[] =
{
#ifdef _IOC_NONE
    { "_IOC_NONE", _IOC_NONE },
#endif
#ifdef _IOC_WRITE
    { "_IOC_WRITE", _IOC_WRITE },
#endif
#ifdef _IOC_READ
    { "_IOC_READ", _IOC_READ },
#endif
#ifdef IOC_VOID
    { "IOC_VOID", IOC_VOID },
#endif
#ifdef IOC_OUT
    { "IOC_OUT", IOC_OUT },
#endif
#ifdef IOC_IN
    { "IOC_IN", IOC_IN },
#endif
#ifdef IOC_INOUT
    { "IOC_INOUT", IOC_INOUT },
#endif
};


static void
explain_buffer_ioc_dir(explain_string_buffer_t *sb, int ioc_dir)
{
    const explain_parse_bits_table_t *tp;

    tp =
        explain_parse_bits_find_by_value
        (
            ioc_dir,
            ioc_dir_table,
            SIZEOF(ioc_dir_table)
        );
    if (tp)
        explain_string_buffer_puts(sb, tp->name);
    else
        explain_string_buffer_printf(sb, "%d", ioc_dir);
}


static void
char_or_hex(explain_string_buffer_t *sb, int value)
{
    if (value >= 0 && value < 256 && isprint(value))
        explain_string_buffer_putc_quoted(sb, value);
    else
        explain_string_buffer_printf(sb, "0x%02X", value);
}


static void
tsize(explain_string_buffer_t *sb, int nbytes)
{
    div_t           d;

    if (nbytes == 0)
    {
        explain_string_buffer_puts(sb, "char[0]");
        return;
    }
    d = div(nbytes, sizeof(int));
    if (d.rem == 0)
    {
        if (d.quot == 1)
            explain_string_buffer_puts(sb, "int");
        else
            explain_string_buffer_printf(sb, "int[%d]", d.quot);
        return;
    }
    d = div(nbytes, sizeof(long));
    if (d.rem == 0)
    {
        if (d.quot == 1)
            explain_string_buffer_puts(sb, "long");
        else
            explain_string_buffer_printf(sb, "long[%d]", d.quot);
        return;
    }
    d = div(nbytes, sizeof(short));
    if (d.rem == 0)
    {
        if (d.quot == 1)
            explain_string_buffer_puts(sb, "short");
        else
            explain_string_buffer_printf(sb, "short[%d]", d.quot);
        return;
    }
    explain_string_buffer_printf(sb, "char[%d]", nbytes);
}


static void
io(explain_string_buffer_t *sb, int type, int nr)
{
    explain_string_buffer_puts(sb, "_IO(");
    char_or_hex(sb, type);
    explain_string_buffer_printf(sb, ", %d)", nr);
}


static void
ioc(explain_string_buffer_t *sb, int dir, int type, int nr, int size)
{
    explain_string_buffer_puts(sb, "_IOC(");
    explain_buffer_ioc_dir(sb, dir);
    explain_string_buffer_puts(sb, ", ");
    char_or_hex(sb, type);
    explain_string_buffer_printf(sb, ", %d, ", nr);
    tsize(sb, size);
    explain_string_buffer_putc(sb, ')');
}


static void
ior(explain_string_buffer_t *sb, int type, int nr, int size)
{
    explain_string_buffer_puts(sb, "_IOR(");
    char_or_hex(sb, type);
    explain_string_buffer_printf(sb, ", %d, ", nr);
    tsize(sb, size);
    explain_string_buffer_putc(sb, ')');
}


static void
iow(explain_string_buffer_t *sb, int type, int nr, int size)
{
    explain_string_buffer_puts(sb, "_IOW(");
    char_or_hex(sb, type);
    explain_string_buffer_printf(sb, ", %d, ", nr);
    tsize(sb, size);
    explain_string_buffer_putc(sb, ')');
}


static void
iorw(explain_string_buffer_t *sb, int type, int nr, int size)
{
    explain_string_buffer_puts(sb, "_IORW(");
    char_or_hex(sb, type);
    explain_string_buffer_printf(sb, ", %d, ", nr);
    tsize(sb, size);
    explain_string_buffer_putc(sb, ')');
}

#endif


void
explain_iocontrol_generic_print_hash_define(explain_string_buffer_t *sb,
    int request)
{
#ifdef SIOCDEVPRIVATE
    if (SIOCDEVPRIVATE <= request && request < SIOCDEVPRIVATE + 16)
    {
        explain_string_buffer_printf
        (
            sb,
            "SIOCDEVPRIVATE + %d",
            request - SIOCDEVPRIVATE
        );
        return;
    }
#endif
#ifdef SIOCPROTOPRIVATE
    if (SIOCPROTOPRIVATE <= request && request < SIOCPROTOPRIVATE + 16)
    {
        explain_string_buffer_printf
        (
            sb,
            "SIOCPROTOPRIVATE + %d",
            request - SIOCPROTOPRIVATE
        );
        return;
    }
#endif
    if (request == 0)
    {
        /* special case for zero */
        explain_string_buffer_putc(sb, '0');
        return;
    }
#if defined(_IOC_DIR) || defined(IOC_DIRMASK)
    {
        int             ioc_dir;
        int             ioc_type;
        int             ioc_nr;
        int             ioc_size;

#ifdef _IOC_DIR
        ioc_dir = _IOC_DIR(request);
#else
        ioc_dir = request & IOC_DIRMASK;
#endif
#ifdef _IOC_TYPE
        ioc_type = _IOC_TYPE(request);
#else
        ioc_type = IOCGROUP(request);
#endif
#ifdef _IOC_NR
        ioc_nr = _IOC_NR(request);
#else
        ioc_nr = request & 0xFF;
#endif
#ifdef _IOC_SIZE
        ioc_size = _IOC_SIZE(request);
#else
        ioc_size = IOCPARM_LEN(request);
#endif
        switch (ioc_dir)
        {
#ifdef _IOC_NONE
        case _IOC_NONE:
#endif
#ifdef IOC_VOID
        case IOC_VOID:
#endif
            if (ioc_size != 0)
                ioc(sb, ioc_dir, ioc_type, ioc_nr, ioc_size);
            else
                io(sb, ioc_type, ioc_nr);
            return;

#ifdef _IOC_READ
        case _IOC_READ:
#endif
#ifdef IOC_OUT
        case IOC_OUT:
#endif
            if (ioc_size != 0)
            {
                ior(sb, ioc_type, ioc_nr, ioc_size);
                return;
            }
            break;

#ifdef _IOC_WRITE
        case _IOC_WRITE:
#endif
#ifdef IOC_IN
        case IOC_IN:
#endif
            if (ioc_size != 0)
            {
                iow(sb, ioc_type, ioc_nr, ioc_size);
                return;
            }
            break;

#ifdef IOC_INOUT
        case IOC_INOUT:
#endif
#if defined(_IOC_READ) && defined(_IOC_WRITE)
        case _IOC_READ | _IOC_WRITE:
#endif
            if (ioc_size != 0)
            {
                iorw(sb, ioc_type, ioc_nr, ioc_size);
                return;
            }
            break;

        default:
            if (ioc_size != 0)
            {
                ioc(sb, ioc_dir, ioc_type, ioc_nr, ioc_size);
                return;
            }
            break;
        }
    }
#endif
    {
        int             prec;

        prec = 2;
        if (request >= 0x10000)
            prec = 8;
        else if (request >= 0x100)
            prec = 4;
        explain_string_buffer_printf(sb, "0x%.*X", prec, request);
    }
}


static void
print_name(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)data;
    explain_iocontrol_generic_print_hash_define(sb, request);
}


void
explain_iocontrol_fake_syscall_name(char *name, int name_size,
    const explain_iocontrol_t *p, int request)
{
    explain_string_buffer_t name_buf;

    explain_string_buffer_init(&name_buf, name, name_size);
    explain_string_buffer_puts(&name_buf, "ioctl ");
    if (!p)
        explain_iocontrol_generic_print_hash_define(&name_buf, request);
    else if (p->name)
        explain_string_buffer_puts(&name_buf, p->name);
    else if (p->print_name)
        p->print_name(p, &name_buf, 0, 0, request, 0);
    else
        explain_iocontrol_generic_print_hash_define(&name_buf, request);
}


void
explain_iocontrol_generic_print_explanation(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    char            name[40];

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/ioctl.html
     */
    (void)data;
    switch (errnum)
    {
    case EACCES:
        explain_iocontrol_fake_syscall_name(name, sizeof(name), p, request);
        explain_buffer_eacces_syscall(sb, name);
        explain_buffer_dac_sys_admin(sb);
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EBUSY:
        explain_iocontrol_fake_syscall_name(name, sizeof(name), p, request);
        explain_buffer_ebusy_fildes(sb, fildes, "fildes", name);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EIO:
        explain_buffer_eio_fildes(sb, fildes);
        break;

    case EINVAL:
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            break;
        }
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This message is used to explain an EINVAL
             * error reported by an ioctl(2) system call, when a more
             * specific explanation is not availble.
             */
            i18n("ioctl request or ioctl data is not valid")
            /*
             * We can't exclude "ioctl request or" from the text, because Linux
             * is somewhat schitzophrenic is this regard, and annoyingly returns
             * EINVAL in many situtions where ENOTTY is patently more suitable.
             *
             * We could, of course, put #ifdef __linux__ around it, since this
             * brain damage is mostly limited to Linux.
             */
        );
        break;

#ifdef ENOMEDIUM
    case ENOMEDIUM:
        explain_buffer_enomedium_fildes(sb, fildes);
        break;
#endif

    case ENOTTY:
    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
#if defined(ENOIOCTLCMD) && ENOIOCTLCMD != ENOTTY
        /*
         * The Linux kernel (well, parts of it) annoyingly returns EINVAL
         * in many situtions where ENOTTY is patently more suitable.
         * Linus Torvalds suggested that the correct course of action is
         *
         *     #define ENOIOCTLCMD ENOTTY
         *
         * and return this value to user space.
         * http://article.gmane.org/gmane.linux.kernel/1160917
         */
    case ENOIOCTLCMD:
#endif
#if defined(ENOIOCTL) && ENOIOCTL != ENOTTY
    case ENOIOCTL:
#endif
        explain_iocontrol_fake_syscall_name(name, sizeof(name), p, request);
        explain_buffer_enosys_fildes(sb, fildes, "fildes", name);
        break;

    case EROFS:
        explain_buffer_erofs_fildes(sb, fildes, "fildes");
        break;

    default:
        explain_iocontrol_fake_syscall_name(name, sizeof(name), p, request);
        explain_buffer_errno_generic(sb, errnum, name);
        break;
    }
}


/*
 * The third argument is an untyped pointer to memory.  It's
 * traditionally char *argp (from the days before "void *" was valid C).
 * So for undefined ioctl requets, we will interpret and print a generic
 * pointer for the third argument (print_data).
 */
const explain_iocontrol_t explain_iocontrol_generic =
{
    0, /* name */
    0, /* value */
    0, /* disambiguate */
    print_name,
    explain_iocontrol_generic_print_data_pointer, /* print data */
    explain_iocontrol_generic_print_explanation, /* print_explanation */
    0, /* print_data_returned */
    VOID_STAR, /* data_size */
    "void *", /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};


void
explain_iocontrol_generic_print_data_int(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int(sb, (intptr_t)data);
}


void
explain_iocontrol_generic_print_data_uint(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_uint(sb, (intptr_t)data);
}


void
explain_iocontrol_generic_print_data_int_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int_star(sb, data);
}


void
explain_iocontrol_generic_print_data_uint_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_uint_star(sb, data);
}


void
explain_iocontrol_generic_print_data_long(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_long(sb, (intptr_t)data);
}


void
explain_iocontrol_generic_print_data_ulong(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ulong(sb, (intptr_t)data);
}


void
explain_iocontrol_generic_print_data_long_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_long_star(sb, data);
}


void
explain_iocontrol_generic_print_data_ulong_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ulong_star(sb, data);
}


void
explain_iocontrol_generic_print_data_pointer(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_pointer(sb, data);
}


void
explain_iocontrol_generic_print_data_int64_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int64_array(sb, data, 1);
}


void
explain_iocontrol_generic_print_data_uint64_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_uint64_array(sb, data, 1);
}


void
explain_iocontrol_generic_print_data_ignored(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_pointer(sb, data);
    if (data)
        explain_string_buffer_puts(sb, " ignored");
}


void
explain_iocontrol_generic_print_data_short_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_short_star(sb, data, 1);
}


void
explain_iocontrol_generic_print_data_ushort_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ushort_star(sb, data, 1);
}


void
explain_iocontrol_generic_print_data_int8_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_int8_star(sb, data, 1);
}


void
explain_iocontrol_generic_print_data_uint8_star(const explain_iocontrol_t *p,
    explain_string_buffer_t *sb, int errnum, int fildes, int request,
    const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_uint8_star(sb, data, 1);
}


/* vim: set ts=8 sw=4 et : */
