/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/ptrace.h>

#include <libexplain/buffer/ptrace_request.h>
#include <libexplain/parse_bits.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef PT_TRACE_ME
    { "PT_TRACE_ME", PT_TRACE_ME },
#endif
#if defined(PTRACE_TRACEME)
    { "PTRACE_TRACEME", PTRACE_TRACEME },
#endif

#ifdef PT_READ_I
    { "PT_READ_I", PT_READ_I },
#endif
#if defined(PTRACE_PEEKTEXT)
    { "PTRACE_PEEKTEXT", PTRACE_PEEKTEXT },
#endif

#ifdef PT_READ_D
    { "PT_READ_D", PT_READ_D },
#endif
#if defined(PTRACE_PEEKDATA)
    { "PTRACE_PEEKDATA", PTRACE_PEEKDATA },
#endif

#ifdef PT_READ_U
    { "PT_READ_U", PT_READ_U },
#endif
#if defined(PTRACE_PEEKUSER)
    { "PTRACE_PEEKUSER", PTRACE_PEEKUSER },
#endif

#ifdef PT_WRITE_I
    { "PT_WRITE_I", PT_WRITE_I },
#endif
#if defined(PTRACE_POKETEXT)
    { "PTRACE_POKETEXT", PTRACE_POKETEXT },
#endif

#ifdef PT_WRITE_D
    { "PT_WRITE_D", PT_WRITE_D },
#endif
#if defined(PTRACE_POKEDATA)
    { "PTRACE_POKEDATA", PTRACE_POKEDATA },
#endif

#ifdef PT_WRITE_U
    { "PT_WRITE_U", PT_WRITE_U },
#endif
#if defined(PTRACE_POKEUSER)
    { "PTRACE_POKEUSER", PTRACE_POKEUSER },
#endif

#ifdef PT_CONTINUE
    { "PT_CONTINUE", PT_CONTINUE },
#endif
#if defined(PTRACE_CONT)
    { "PTRACE_CONT", PTRACE_CONT },
#endif

#ifdef PT_KILL
    { "PT_KILL", PT_KILL },
#endif
#if defined(PTRACE_KILL)
    { "PTRACE_KILL", PTRACE_KILL },
#endif

#ifdef PT_STEP
    { "PT_STEP", PT_STEP },
#endif
#if defined(PTRACE_SINGLESTEP)
    { "PTRACE_SINGLESTEP", PTRACE_SINGLESTEP },
#endif

#ifdef PT_GETREGS
    { "PT_GETREGS", PT_GETREGS },
#endif
#if defined(PTRACE_GETREGS)
    { "PTRACE_GETREGS", PTRACE_GETREGS },
#endif

#ifdef PT_SETREGS
    { "PT_SETREGS", PT_SETREGS },
#endif
#if defined(PTRACE_SETREGS)
    { "PTRACE_SETREGS", PTRACE_SETREGS },
#endif

#ifdef PT_GETFPREGS
    { "PT_GETFPREGS", PT_GETFPREGS },
#endif
#if defined(PTRACE_GETFPREGS)
    { "PTRACE_GETFPREGS", PTRACE_GETFPREGS },
#endif

#ifdef PT_SETFPREGS
    { "PT_SETFPREGS", PT_SETFPREGS },
#endif
#if defined(PTRACE_SETFPREGS)
    { "PTRACE_SETFPREGS", PTRACE_SETFPREGS },
#endif

#ifdef PT_ATTACH
    { "PT_ATTACH", PT_ATTACH },
#endif
#if defined(PTRACE_ATTACH)
    { "PTRACE_ATTACH", PTRACE_ATTACH },
#endif

#ifdef PT_DETACH
    { "PT_DETACH", PT_DETACH },
#endif
#if defined(PTRACE_DETACH)
    { "PTRACE_DETACH", PTRACE_DETACH },
#endif

#ifdef PT_IO
    { "PT_IO", PT_IO },
#endif
#ifdef PT_LWPINFO
    { "PT_LWPINFO", PT_LWPINFO },
#endif
#ifdef PT_GETNUMLWPS
    { "PT_GETNUMLWPS", PT_GETNUMLWPS },
#endif
#ifdef PT_GETLWPLIST
    { "PT_GETLWPLIST", PT_GETLWPLIST },
#endif
#ifdef PT_CLEARSTEP
    { "PT_CLEARSTEP", PT_CLEARSTEP },
#endif
#ifdef PT_SETSTEP
    { "PT_SETSTEP", PT_SETSTEP },
#endif
#ifdef PT_SUSPEND
    { "PT_SUSPEND", PT_SUSPEND },
#endif
#ifdef PT_RESUME
    { "PT_RESUME", PT_RESUME },
#endif
#ifdef PT_TO_SCE
    { "PT_TO_SCE", PT_TO_SCE },
#endif
#ifdef PT_TO_SCX
    { "PT_TO_SCX ", PT_TO_SCX  },
#endif
#ifdef PT_GETDBREGS
    { "PT_GETDBREGS", PT_GETDBREGS },
#endif
#ifdef PT_SETDBREGS
    { "PT_SETDBREGS", PT_SETDBREGS },
#endif
#ifdef PT_VM_TIMESTAMP
    { "PT_VM_TIMESTAMP", PT_VM_TIMESTAMP },
#endif
#ifdef PT_VM_ENTRY
    { "PT_VM_ENTRY", PT_VM_ENTRY },
#endif

#ifdef PT_GETFPXREGS
    { "PT_GETFPXREGS", PT_GETFPXREGS },
#endif
#if defined(PTRACE_GETFPXREGS)
    { "PTRACE_GETFPXREGS", PTRACE_GETFPXREGS },
#endif

#ifdef TP_SETFPXREGS
    { "TP_SETFPXREGS", TP_SETFPXREGS },
#endif
#if defined(PTRACE_SETFPXREGS)
    { "PTRACE_SETFPXREGS", PTRACE_SETFPXREGS },
#endif

#ifdef PT_SYSCALL
    { "PT_SYSCALL", PT_SYSCALL },
#endif
#if defined(PTRACE_SYSCALL)
    { "PTRACE_SYSCALL", PTRACE_SYSCALL },
#endif

#ifdef PT_SETOPTIONS
    { "PT_SETOPTIONS", PT_SETOPTIONS },
#endif
#if defined(PTRACE_SETOPTIONS)
    { "PTRACE_SETOPTIONS", PTRACE_SETOPTIONS },
#endif

#ifdef PT_OLDSETOPTIONS
    { "PT_OLDSETOPTIONS", PT_OLDSETOPTIONS },
#endif
#if defined(PTRACE_OLDSETOPTIONS)
    { "PTRACE_OLDSETOPTIONS", PTRACE_OLDSETOPTIONS },
#endif

#ifdef PT_GETEVENTMSG
    { "PT_GETEVENTMSG", PT_GETEVENTMSG },
#endif
#if defined(PTRACE_GETEVENTMSG)
    { "PTRACE_GETEVENTMSG", PTRACE_GETEVENTMSG },
#endif

#ifdef PT_GETSIGINFO
    { "PT_GETSIGINFO", PT_GETSIGINFO },
#endif
#if defined(PTRACE_GETSIGINFO)
    { "PTRACE_GETSIGINFO", PTRACE_GETSIGINFO },
#endif

#ifdef PT_SETSIGINFO
    { "PT_SETSIGINFO", PT_SETSIGINFO },
#endif
#if defined(PTRACE_SETSIGINFO)
    { "PTRACE_SETSIGINFO", PTRACE_SETSIGINFO },
#endif

#ifdef PT_SYSEMU
    { "PT_SYSEMU", PT_SYSEMU },
#endif
#if defined(PTRACE_SYSEMU)
    { "PTRACE_SYSEMU", PTRACE_SYSEMU },
#endif

#ifdef PT_SYSEMU_SINGLESTEP
    { "PT_SYSEMU_SINGLESTEP", PT_SYSEMU_SINGLESTEP },
#endif
#if defined(PTRACE_SYSEMU_SINGLESTEP)
    { "PTRACE_SYSEMU_SINGLESTEP", PTRACE_SYSEMU_SINGLESTEP },
#endif
};


void
explain_buffer_ptrace_request(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


int
explain_parse_ptrace_request_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
