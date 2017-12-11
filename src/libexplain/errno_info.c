/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/errno.h>

#include <libexplain/errno_info/table.h>
#include <libexplain/sizeof.h>


const explain_errno_info_t explain_errno_info[] =
{
#ifdef EPERM
    {
        EPERM,
        "EPERM",
        "Operation not permitted"
    },
#endif
#ifdef ENOENT
    {
        ENOENT,
        "ENOENT",
        "No such file or directory"
    },
#endif
#ifdef ESRCH
    {
        ESRCH,
        "ESRCH",
        "No such process"
    },
#endif
#ifdef EINTR
    {
        EINTR,
        "EINTR",
        "Interrupted system call"
    },
#endif
#ifdef EIO
    {
        EIO,
        "EIO",
        "Input/output error"
    },
#endif
#ifdef ENXIO
    {
        ENXIO,
        "ENXIO",
        "No such device or address"
    },
#endif
#ifdef E2BIG
    {
        E2BIG,
        "E2BIG",
        "Argument list too long"
    },
#endif
#ifdef ENOEXEC
    {
        ENOEXEC,
        "ENOEXEC",
        "Exec format error"
    },
#endif
#ifdef EBADF
    {
        EBADF,
        "EBADF",
        "Bad file descriptor"
    },
#endif
#ifdef ECHILD
    {
        ECHILD,
        "ECHILD",
        "No child processes"
    },
#endif
#ifdef EAGAIN
    {
        EAGAIN,
        "EAGAIN",
        "Resource temporarily unavailable"
    },
#endif
#ifdef ENOMEM
    {
        ENOMEM,
        "ENOMEM",
        "Cannot allocate memory"
    },
#endif
#ifdef EACCES
    {
        EACCES,
        "EACCES",
        "Permission denied"
    },
#endif
#ifdef EFAULT
    {
        EFAULT,
        "EFAULT",
        "Bad address"
    },
#endif
#ifdef ENOTBLK
    {
        ENOTBLK,
        "ENOTBLK",
        "Block device required"
    },
#endif
#ifdef EBUSY
    {
        EBUSY,
        "EBUSY",
        "Device or resource busy"
    },
#endif
#ifdef EEXIST
    {
        EEXIST,
        "EEXIST",
        "File exists"
    },
#endif
#ifdef EXDEV
    {
        EXDEV,
        "EXDEV",
        "Invalid cross-device link"
    },
#endif
#ifdef ENODEV
    {
        ENODEV,
        "ENODEV",
        "No such device"
    },
#endif
#ifdef ENOTDIR
    {
        ENOTDIR,
        "ENOTDIR",
        "Not a directory"
    },
#endif
#ifdef EISDIR
    {
        EISDIR,
        "EISDIR",
        "Is a directory"
    },
#endif
#ifdef EINVAL
    {
        EINVAL,
        "EINVAL",
        "Invalid argument"
    },
#endif
#ifdef ENFILE
    {
        ENFILE,
        "ENFILE",
        "Too many open files in system"
    },
#endif
#ifdef EMFILE
    {
        EMFILE,
        "EMFILE",
        "Too many open files"
    },
#endif
#ifdef ENOTTY
    {
        ENOTTY,
        "ENOTTY",
        "Inappropriate ioctl for device"
    },
#endif
#ifdef ETXTBSY
    {
        ETXTBSY,
        "ETXTBSY",
        "Text file busy"
    },
#endif
#ifdef EFBIG
    {
        EFBIG,
        "EFBIG",
        "File too large"
    },
#endif
#ifdef ENOSPC
    {
        ENOSPC,
        "ENOSPC",
        "No space left on device"
    },
#endif
#ifdef ESPIPE
    {
        ESPIPE,
        "ESPIPE",
        "Illegal seek"
    },
#endif
#ifdef EROFS
    {
        EROFS,
        "EROFS",
        "Read-only file system"
    },
#endif
#ifdef EMLINK
    {
        EMLINK,
        "EMLINK",
        "Too many links"
    },
#endif
#ifdef EPIPE
    {
        EPIPE,
        "EPIPE",
        "Broken pipe"
    },
#endif
#ifdef EDOM
    {
        EDOM,
        "EDOM",
        "Numerical argument out of domain"
    },
#endif
#ifdef ERANGE
    {
        ERANGE,
        "ERANGE",
        "Numerical result out of range"
    },
#endif
#ifdef EDEADLK
    {
        EDEADLK,
        "EDEADLK",
        "Resource deadlock avoided"
    },
#endif
#ifdef EDEADLOCK
    {
        EDEADLOCK,
        "EDEADLOCK",
        "Resource deadlock avoided"
    },
#endif
#ifdef ENAMETOOLONG
    {
        ENAMETOOLONG,
        "ENAMETOOLONG",
        "File name too long"
    },
#endif
#ifdef ENOLCK
    {
        ENOLCK,
        "ENOLCK",
        "No locks available"
    },
#endif
#ifdef ENOSYS
    {
        ENOSYS,
        "ENOSYS",
        "Function not implemented"
    },
#endif
#ifdef ENOTEMPTY
    {
        ENOTEMPTY,
        "ENOTEMPTY",
        "Directory not empty"
    },
#endif
#ifdef ELOOP
    {
        ELOOP,
        "ELOOP",
        "Too many levels of symbolic links"
    },
#endif
#ifdef EWOULDBLOCK
    {
        EWOULDBLOCK,
        "EWOULDBLOCK",
        "Resource temporarily unavailable"
    },
#endif
#ifdef ENOMSG
    {
        ENOMSG,
        "ENOMSG",
        "No message of desired type"
    },
#endif
#ifdef EIDRM
    {
        EIDRM,
        "EIDRM",
        "Identifier removed"
    },
#endif
#ifdef ECHRNG
    {
        ECHRNG,
        "ECHRNG",
        "Channel number out of range"
    },
#endif
#ifdef EL2NSYNC
    {
        EL2NSYNC,
        "EL2NSYNC",
        "Level 2 not synchronized"
    },
#endif
#ifdef EL3HLT
    {
        EL3HLT,
        "EL3HLT",
        "Level 3 halted"
    },
#endif
#ifdef EL3RST
    {
        EL3RST,
        "EL3RST",
        "Level 3 reset"
    },
#endif
#ifdef ELNRNG
    {
        ELNRNG,
        "ELNRNG",
        "Link number out of range"
    },
#endif
#ifdef EUNATCH
    {
        EUNATCH,
        "EUNATCH",
        "Protocol driver not attached"
    },
#endif
#ifdef ENOCSI
    {
        ENOCSI,
        "ENOCSI",
        "No CSI structure available"
    },
#endif
#ifdef EL2HLT
    {
        EL2HLT,
        "EL2HLT",
        "Level 2 halted"
    },
#endif
#ifdef EBADE
    {
        EBADE,
        "EBADE",
        "Invalid exchange"
    },
#endif
#ifdef EBADR
    {
        EBADR,
        "EBADR",
        "Invalid request descriptor"
    },
#endif
#ifdef EXFULL
    {
        EXFULL,
        "EXFULL",
        "Exchange full"
    },
#endif
#ifdef ENOANO
    {
        ENOANO,
        "ENOANO",
        "No anode"
    },
#endif
#ifdef EBADRQC
    {
        EBADRQC,
        "EBADRQC",
        "Invalid request code"
    },
#endif
#ifdef EBADSLT
    {
        EBADSLT,
        "EBADSLT",
        "Invalid slot"
    },
#endif
#ifdef EBFONT
    {
        EBFONT,
        "EBFONT",
        "Bad font file format"
    },
#endif
#ifdef ENOSTR
    {
        ENOSTR,
        "ENOSTR",
        "Device not a stream"
    },
#endif
#ifdef ENODATA
    {
        ENODATA,
        "ENODATA",
        "No data available"
    },
#endif
#ifdef ETIME
    {
        ETIME,
        "ETIME",
        "Timer expired"
    },
#endif
#ifdef ENOSR
    {
        ENOSR,
        "ENOSR",
        "Out of streams resources"
    },
#endif
#ifdef ENONET
    {
        ENONET,
        "ENONET",
        "Machine is not on the network"
    },
#endif
#ifdef ENOPKG
    {
        ENOPKG,
        "ENOPKG",
        "Package not installed"
    },
#endif
#ifdef EREMOTE
    {
        EREMOTE,
        "EREMOTE",
        "Object is remote"
    },
#endif
#ifdef ENOLINK
    {
        ENOLINK,
        "ENOLINK",
        "Link has been severed"
    },
#endif
#ifdef EADV
    {
        EADV,
        "EADV",
        "Advertise error"
    },
#endif
#ifdef ESRMNT
    {
        ESRMNT,
        "ESRMNT",
        "Srmount error"
    },
#endif
#ifdef ECOMM
    {
        ECOMM,
        "ECOMM",
        "Communication error on send"
    },
#endif
#ifdef EPROTO
    {
        EPROTO,
        "EPROTO",
        "Protocol error"
    },
#endif
#ifdef EMULTIHOP
    {
        EMULTIHOP,
        "EMULTIHOP",
        "Multihop attempted"
    },
#endif
#ifdef EDOTDOT
    {
        EDOTDOT,
        "EDOTDOT",
        "RFS specific error"
    },
#endif
#ifdef EBADMSG
    {
        EBADMSG,
        "EBADMSG",
        "Bad message"
    },
#endif
#ifdef EOVERFLOW
    {
        EOVERFLOW,
        "EOVERFLOW",
        "Value too large for defined data type"
    },
#endif
#ifdef ENOTUNIQ
    {
        ENOTUNIQ,
        "ENOTUNIQ",
        "Name not unique on network"
    },
#endif
#ifdef EBADFD
    {
        EBADFD,
        "EBADFD",
        "File descriptor in bad state"
    },
#endif
#ifdef EREMCHG
    {
        EREMCHG,
        "EREMCHG",
        "Remote address changed"
    },
#endif
#ifdef ELIBACC
    {
        ELIBACC,
        "ELIBACC",
        "Can not access a needed shared library"
    },
#endif
#ifdef ELIBBAD
    {
        ELIBBAD,
        "ELIBBAD",
        "Accessing a corrupted shared library"
    },
#endif
#ifdef ELIBSCN
    {
        ELIBSCN,
        "ELIBSCN",
        ".lib section in a.out corrupted"
    },
#endif
#ifdef ELIBMAX
    {
        ELIBMAX,
        "ELIBMAX",
        "Attempting to link in too many shared libraries"
    },
#endif
#ifdef ELIBEXEC
    {
        ELIBEXEC,
        "ELIBEXEC",
        "Cannot exec a shared library directly"
    },
#endif
#ifdef EILSEQ
    {
        EILSEQ,
        "EILSEQ",
        "Invalid or incomplete multibyte or wide character"
    },
#endif
#ifdef ERESTART
    {
        ERESTART,
        "ERESTART",
        "Interrupted system call should be restarted"
    },
#endif
#ifdef ESTRPIPE
    {
        ESTRPIPE,
        "ESTRPIPE",
        "Streams pipe error"
    },
#endif
#ifdef EUSERS
    {
        EUSERS,
        "EUSERS",
        "Too many users"
    },
#endif
#ifdef ENOTSOCK
    {
        ENOTSOCK,
        "ENOTSOCK",
        "Socket operation on non-socket"
    },
#endif
#ifdef EDESTADDRREQ
    {
        EDESTADDRREQ,
        "EDESTADDRREQ",
        "Destination address required"
    },
#endif
#ifdef EMSGSIZE
    {
        EMSGSIZE,
        "EMSGSIZE",
        "Message too long"
    },
#endif
#ifdef EPROTOTYPE
    {
        EPROTOTYPE,
        "EPROTOTYPE",
        "Protocol wrong type for socket"
    },
#endif
#ifdef ENOPROTOOPT
    {
        ENOPROTOOPT,
        "ENOPROTOOPT",
        "Protocol not available"
    },
#endif
#ifdef EPROTONOSUPPORT
    {
        EPROTONOSUPPORT,
        "EPROTONOSUPPORT",
        "Protocol not supported"
    },
#endif
#ifdef ESOCKTNOSUPPORT
    {
        ESOCKTNOSUPPORT,
        "ESOCKTNOSUPPORT",
        "Socket type not supported"
    },
#endif
#ifdef EOPNOTSUPP
    {
        EOPNOTSUPP,
        "EOPNOTSUPP",
        "Operation not supported"
    },
#endif
#ifdef EPFNOSUPPORT
    {
        EPFNOSUPPORT,
        "EPFNOSUPPORT",
        "Protocol family not supported"
    },
#endif
#ifdef EAFNOSUPPORT
    {
        EAFNOSUPPORT,
        "EAFNOSUPPORT",
        "Address family not supported by protocol"
    },
#endif
#ifdef EADDRINUSE
    {
        EADDRINUSE,
        "EADDRINUSE",
        "Address already in use"
    },
#endif
#ifdef EADDRNOTAVAIL
    {
        EADDRNOTAVAIL,
        "EADDRNOTAVAIL",
        "Cannot assign requested address"
    },
#endif
#ifdef ENETDOWN
    {
        ENETDOWN,
        "ENETDOWN",
        "Network is down"
    },
#endif
#ifdef ENETUNREACH
    {
        ENETUNREACH,
        "ENETUNREACH",
        "Network is unreachable"
    },
#endif
#ifdef ENETRESET
    {
        ENETRESET,
        "ENETRESET",
        "Network dropped connection on reset"
    },
#endif
#ifdef ECONNABORTED
    {
        ECONNABORTED,
        "ECONNABORTED",
        "Software caused connection abort"
    },
#endif
#ifdef ECONNRESET
    {
        ECONNRESET,
        "ECONNRESET",
        "Connection reset by peer"
    },
#endif
#ifdef ENOBUFS
    {
        ENOBUFS,
        "ENOBUFS",
        "No buffer space available"
    },
#endif
#ifdef EISCONN
    {
        EISCONN,
        "EISCONN",
        "Transport endpoint is already connected"
    },
#endif
#ifdef ENOTCONN
    {
        ENOTCONN,
        "ENOTCONN",
        "Transport endpoint is not connected"
    },
#endif
#ifdef ESHUTDOWN
    {
        ESHUTDOWN,
        "ESHUTDOWN",
        "Cannot send after transport endpoint shutdown"
    },
#endif
#ifdef ETOOMANYREFS
    {
        ETOOMANYREFS,
        "ETOOMANYREFS",
        "Too many references: cannot splice"
    },
#endif
#ifdef ETIMEDOUT
    {
        ETIMEDOUT,
        "ETIMEDOUT",
        "Connection timed out"
    },
#endif
#ifdef ECONNREFUSED
    {
        ECONNREFUSED,
        "ECONNREFUSED",
        "Connection refused"
    },
#endif
#ifdef EHOSTDOWN
    {
        EHOSTDOWN,
        "EHOSTDOWN",
        "Host is down"
    },
#endif
#ifdef EHOSTUNREACH
    {
        EHOSTUNREACH,
        "EHOSTUNREACH",
        "No route to host"
    },
#endif
#ifdef EALREADY
    {
        EALREADY,
        "EALREADY",
        "Operation already in progress"
    },
#endif
#ifdef EINPROGRESS
    {
        EINPROGRESS,
        "EINPROGRESS",
        "Operation now in progress"
    },
#endif
#ifdef ESTALE
    {
        ESTALE,
        "ESTALE",
        "Stale NFS file handle"
    },
#endif
#ifdef EUCLEAN
    {
        EUCLEAN,
        "EUCLEAN",
        "Structure needs cleaning"
    },
#endif
#ifdef ENOTNAM
    {
        ENOTNAM,
        "ENOTNAM",
        "Not a XENIX named type file"
    },
#endif
#ifdef ENAVAIL
    {
        ENAVAIL,
        "ENAVAIL",
        "No XENIX semaphores available"
    },
#endif
#ifdef EISNAM
    {
        EISNAM,
        "EISNAM",
        "Is a named type file"
    },
#endif
#ifdef EREMOTEIO
    {
        EREMOTEIO,
        "EREMOTEIO",
        "Remote I/O error"
    },
#endif
#ifdef EDQUOT
    {
        EDQUOT,
        "EDQUOT",
        "Disk quota exceeded"
    },
#endif
#ifdef ENOMEDIUM
    {
        ENOMEDIUM,
        "ENOMEDIUM",
        "No medium found"
    },
#endif
#ifdef EMEDIUMTYPE
    {
        EMEDIUMTYPE,
        "EMEDIUMTYPE",
        "Wrong medium type"
    },
#endif
#ifdef ECANCELED
    {
        ECANCELED,
        "ECANCELED",
        "Operation canceled"
    },
#endif
#ifdef ENOKEY
    {
        ENOKEY,
        "ENOKEY",
        "Required key not available"
    },
#endif
#ifdef EKEYEXPIRED
    {
        EKEYEXPIRED,
        "EKEYEXPIRED",
        "Key has expired"
    },
#endif
#ifdef EKEYREVOKED
    {
        EKEYREVOKED,
        "EKEYREVOKED",
        "Key has been revoked"
    },
#endif
#ifdef EKEYREJECTED
    {
        EKEYREJECTED,
        "EKEYREJECTED",
        "Key was rejected by service"
    },
#endif
#ifdef EOWNERDEAD
    {
        EOWNERDEAD,
        "EOWNERDEAD",
        "Owner died"
    },
#endif
#ifdef ENOTRECOVERABLE
    {
        ENOTRECOVERABLE,
        "ENOTRECOVERABLE",
        "State not recoverable"
    },
#endif
#ifdef ELIBAD
    { ELIBAD, "ELIBAD", 0 }, /* FreeBSD */
#endif
#ifdef EAUTH
    { EAUTH, "EAUTH", 0 }, /* FreeBSD */
#endif
#ifdef EBADRPC
    { EBADRPC, "EBADRPC", 0 }, /* FreeBSD */
#endif
#ifdef EDIRIOCTL
    { EDIRIOCTL, "EDIRIOCTL", 0 }, /* pseudo, FreeBSD */
#endif
#ifdef EDOOFUS
    { EDOOFUS, "EDOOFUS", 0 }, /* FreeBSD */
#endif
#ifdef EFTYPE
    { EFTYPE, "EFTYPE", 0 }, /* FreeBSD */
#endif
#ifdef EJUSTRETURN
    { EJUSTRETURN, "EJUSTRETURN", 0 }, /* pseudo, FreeBSD */
#endif
#ifdef ENEEDAUTH
    { ENEEDAUTH, "ENEEDAUTH", 0 }, /* FreeBSD */
#endif
#ifdef ENOATTR
    { ENOATTR, "ENOATTR", 0 }, /* FreeBSD */
#endif
#ifdef ENOIOCTL
    { ENOIOCTL, "ENOIOCTL", 0 }, /* pseudo, FreeBSD */
#endif
#ifdef EPROCLIM
    { EPROCLIM, "EPROCLIM", 0 }, /* FreeBSD */
#endif
#ifdef EPROCUNAVAIL
    { EPROCUNAVAIL, "EPROCUNAVAIL", 0 }, /* FreeBSD */
#endif
#ifdef EPROGMISMATCH
    { EPROGMISMATCH, "EPROGMISMATCH", 0 }, /* FreeBSD */
#endif
#ifdef EPROGUNAVAIL
    { EPROGUNAVAIL, "EPROGUNAVAIL", 0 }, /* FreeBSD */
#endif
#ifdef ERPCMISMATCH
    { ERPCMISMATCH, "ERPCMISMATCH", 0 }, /* FreeBSD */
#endif
#ifdef ENOTSUP
    {
        ENOTSUP,
        "ENOTSUP",
        "Operation not supported"
    },
#endif
#ifdef ELAST
    { ELAST, "ELAST", 0 }, /* FreeBSD: "must be equal to largest errno" */
#endif
};

const size_t explain_errno_info_size = SIZEOF(explain_errno_info);


/* vim: set ts=8 sw=4 et : */
