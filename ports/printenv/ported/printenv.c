/*	$OpenBSD: printenv.c,v 1.8 2015/10/09 01:37:08 deraadt Exp $	*/

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* amiport: AmigaOS 3.x port of printenv 1.8
 * environ does not exist in libnix -noixemul. All environment access is
 * done via AmigaDOS GetVar()/ExAll() on ENV: directory instead.
 */

#include <stdio.h>
#include <string.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (activates exit macro) */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
/* amiport: added glob.h for amiport_expand_argv/amiport_free_argv */
#include <amiport/glob.h>

/* amiport: AmigaOS headers for ExAll() ENV: enumeration */
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/exall.h>

/* amiport: pledge() is an OpenBSD sandbox call -- stub as no-op */
#define pledge(p, e) (0)

/* amiport: version string for AmigaOS version command */
static const char *verstag = "$VER: printenv 1.8 (26.03.2026)";

/* amiport: stack cookie -- default 4KB Amiga stack is insufficient */
long __stack = 8192;

/*
 * ENV: value read buffer -- AmigaOS GetVar() requires a caller-provided
 * buffer. 256 bytes covers the vast majority of environment variables.
 * GetVar() truncates silently if the value is longer.
 */
#define ENV_VAL_BUF 256

/*
 * print_all_env -- enumerate all variables in ENV: and print NAME=VALUE
 *
 * Uses ExAll() with AllocDosObject() as required by the ADCD. Iterates
 * over every file in ENV: (AmigaOS stores each variable as a file), reads
 * the value with GetVar(), and prints in the POSIX NAME=VALUE format.
 *
 * ExAll() only returns files at the top level of ENV: -- subdirectories
 * (used for AmigaOS system variables) are skipped with ed_Type < 0.
 * This matches POSIX behavior: printenv shows shell variables, not
 * AmigaOS device/system variables stored in subdirs.
 *
 * amiport: No environ[] on AmigaOS -noixemul. ENV: directory is the
 * environment store. ExAll() + GetVar() is the correct approach.
 */
static void
print_all_env(void)
{
    BPTR lock;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    /* amiport: ED_NAME is sufficient -- we only need filenames to call GetVar() */
    /* Buffer sized for ~40 entries of just names; ExAll loops if needed */
    static UBYTE exall_buf[1024];
    BOOL more;
    char val[ENV_VAL_BUF];
    LONG vlen;

    lock = Lock("ENV:", SHARED_LOCK);
    if (!lock) {
        /* amiport: ENV: may not exist on a minimal AmigaOS setup -- silently succeed */
        return;
    }

    /* amiport: control struct MUST be AllocDosObject per ADCD ExAll docs */
    eac = (struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL, NULL);
    if (!eac) {
        UnLock(lock);
        return;
    }
    eac->eac_LastKey = 0;
    eac->eac_MatchString = NULL;
    eac->eac_MatchFunc = NULL;

    do {
        more = ExAll(lock, (STRPTR)exall_buf, sizeof(exall_buf), ED_NAME, eac);
        if (!more && IoErr() != ERROR_NO_MORE_ENTRIES) {
            /* amiport: ExAll failed abnormally -- stop iteration */
            break;
        }
        if (eac->eac_Entries == 0) {
            /* amiport: no entries in this buffer pass -- loop if more */
            continue;
        }

        ead = (struct ExAllData *)exall_buf;
        do {
            /* amiport: ed_Type < 0 means directory -- skip subdirs.
             * AmigaOS system variables (like ENV:Sys/) live in subdirs
             * and are not user environment variables. */
            /* Note: ED_NAME type does not fill ed_Type (it's 0).
             * We skip entries whose name contains '/' or ':' to avoid
             * printing subdirectory entries mistakenly included. */

            if (ead->ed_Name != NULL) {
                /* amiport: read variable value from ENV: via GetVar() */
                vlen = GetVar((CONST_STRPTR)ead->ed_Name,
                              (STRPTR)val, (LONG)(sizeof(val) - 1),
                              GVF_GLOBAL_ONLY);
                if (vlen >= 0) {
                    val[vlen] = '\0';
                    /* amiport: print in POSIX NAME=VALUE format */
                    printf("%s=%s\n", ead->ed_Name, val);
                }
                /* amiport: skip variables that don't exist globally --
                 * they may be local (task-level) vars not in ENV: */
            }

            ead = ead->ed_Next;
        } while (ead);

    } while (more);

    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(lock);
}

/*
 * printenv
 *
 * Bill Joy, UCB
 * February, 1979
 */
int
main(int argc, char *argv[])
{
    /* amiport: AmigaOS replacement for 'extern char **environ' -- not available */
    char val[ENV_VAL_BUF];
    LONG vlen;
    /* amiport: suppress verstag linker removal */
    (void)verstag;

    /* amiport: expand AmigaOS wildcards in argv (standard boilerplate) */
    amiport_expand_argv(&argc, &argv);
    atexit(amiport_free_argv);

    /* amiport: pledge() stubbed as no-op above -- OpenBSD sandbox API */
    if (pledge("stdio", NULL) == -1)
        err(10, "pledge");

    if (argc < 2) {
        /* amiport: replaced environ[] iteration with ExAll() on ENV: */
        print_all_env();
        exit(0);
    }

    /* amiport: specific variable lookup -- use GetVar() directly.
     * On POSIX, printenv NAME used strncmp on environ entries.
     * On AmigaOS, GetVar() reads the named variable from ENV: directly.
     * GVF_GLOBAL_ONLY matches the POSIX behavior of reading the
     * exported (global) environment. */
    vlen = GetVar((CONST_STRPTR)*++argv, (STRPTR)val, (LONG)(sizeof(val) - 1),
                  GVF_GLOBAL_ONLY);
    if (vlen >= 0) {
        val[vlen] = '\0';
        puts(val);
        exit(0);
    }

    /* amiport: exit(1) -> exit(10): AmigaOS RETURN_ERROR for scripts */
    exit(10);
}
