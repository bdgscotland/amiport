/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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

/*
 * sum.c -- BSD sum algorithms (sum1/sum2) and print functions (pcrc/psum1/psum2).
 *
 * amiport: this file combines FreeBSD's print.c, sum1.c, and sum2.c.
 * These source files were not included in the original/ directory
 * but are required by cksum.c and declared in extern.h.
 */

#include <sys/types.h>
#include <stdio.h>
/* amiport: replaced <unistd.h> with amiport shim -- provides read() macro */
#include <amiport/unistd.h>

#include "extern.h"

/* -----------------------------------------------------------------------
 * pcrc -- print the POSIX CRC checksum result
 * ----------------------------------------------------------------------- */
void
pcrc(char *fn, uint32_t val, off_t len)
{
    (void)printf("%lu %lld", (unsigned long)val, (long long)len);
    if (fn != NULL)
        (void)printf(" %s", fn);
    (void)printf("\n");
}

/* -----------------------------------------------------------------------
 * psum1 -- print BSD sum1 (16-bit rotate-right checksum) result
 * Output format: <sum> <1024-byte blocks> [<filename>]
 * ----------------------------------------------------------------------- */
void
psum1(char *fn, uint32_t val, off_t len)
{
    (void)printf("%05lu %5lld", (unsigned long)val,
        (long long)((len + 1023) / 1024));
    if (fn != NULL)
        (void)printf(" %s", fn);
    (void)printf("\n");
}

/* -----------------------------------------------------------------------
 * psum2 -- print SYSV sum2 (32-bit additive checksum) result
 * Output format: <sum> <512-byte blocks> [<filename>]
 * ----------------------------------------------------------------------- */
void
psum2(char *fn, uint32_t val, off_t len)
{
    (void)printf("%05lu %5lld", (unsigned long)val,
        (long long)((len + 511) / 512));
    if (fn != NULL)
        (void)printf(" %s", fn);
    (void)printf("\n");
}

/* -----------------------------------------------------------------------
 * csum1 -- BSD sum1: 16-bit rotate-right checksum (POSIX -o 1)
 * ----------------------------------------------------------------------- */
int
csum1(int fd, uint32_t *cval, off_t *clen)
{
    uint32_t lcrc;
    LONG nr; /* amiport: LONG matches amiport_read() return type */
    off_t len;
    u_char *p;
    u_char buf[16 * 1024];

    lcrc = len = 0;
    while ((nr = read(fd, buf, sizeof(buf))) > 0) { /* amiport: read() -> amiport_read() via macro */
        len += nr;
        for (p = buf; nr-- > 0; ++p) {
            if (lcrc & 1)
                lcrc |= 0x10000;
            lcrc = (lcrc >> 1) + *p;
            lcrc &= 0xffff;
        }
    }
    if (nr < 0)
        return (1);

    *cval = lcrc;
    *clen = len;
    return (0);
}

/* -----------------------------------------------------------------------
 * csum2 -- SYSV sum2: 32-bit additive checksum (POSIX -o 2)
 * ----------------------------------------------------------------------- */
int
csum2(int fd, uint32_t *cval, off_t *clen)
{
    uint32_t lcrc;
    LONG nr; /* amiport: LONG matches amiport_read() return type */
    off_t len;
    u_char *p;
    u_char buf[16 * 1024];

    lcrc = len = 0;
    while ((nr = read(fd, buf, sizeof(buf))) > 0) { /* amiport: read() -> amiport_read() via macro */
        len += nr;
        for (p = buf; nr-- > 0; ++p)
            lcrc += *p;
    }
    if (nr < 0)
        return (1);

    lcrc = (lcrc & 0xffff) + ((lcrc >> 16) & 0xffff);
    lcrc = (lcrc & 0xffff) + (lcrc >> 16);

    *cval = lcrc;
    *clen = len;
    return (0);
}
