/*      $OpenBSD: regular.c,v 1.13 2021/01/09 09:58:12 otto Exp $      */
/*      $NetBSD: regular.c,v 1.2 1995/09/08 03:22:59 tls Exp $      */

/*-
 * Copyright (c) 1991, 1993, 1994
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
 * amiport-emu: replaced <sys/mman.h> with <amiport-emu/mmap.h>.
 * mmap() emulated via AllocMem()+Read() -- read-only MAP_PRIVATE only.
 * Entire file read into memory upfront; no lazy paging; falls back to
 * c_special() on failure (existing mmap_failed: path).
 * Link with -lamiport-emu in addition to -lamiport.
 */
#include <amiport-emu/mmap.h>
/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h> */
#include <amiport/sys/stat.h>

/* amiport: replaced <err.h> with <amiport/err.h> -- bare <err.h> missing in bebbo-gcc libnix */
#include <amiport/err.h>
/* amiport: removed <stdint.h> -- SIZE_MAX defined locally below; not in libnix C89 headers */
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit()->amiport_exit() macro */
#include <amiport/stdlib.h>
/* amiport: <amiport/unistd.h> for amiport_close() used in mmap_failed fallback */
#include <amiport/unistd.h>
#include <stdio.h>
#include <string.h>

#include "extern.h"

/*
 * amiport: POSIX mmap/munmap/madvise mapped to amiport_emu wrappers.
 * MAP_PRIVATE, PROT_READ, MAP_FAILED mapped to AMIPORT_EMU_* constants.
 * madvise() is a no-op on AmigaOS -- stubbed out.
 */
#define mmap(a, l, p, f, fd, o) \
    amiport_emu_mmap(a, l, p, f, fd, (long)(o))
#define munmap(a, l)    amiport_emu_munmap(a, l)
#define MAP_FAILED      AMIPORT_EMU_MAP_FAILED
#define PROT_READ       AMIPORT_EMU_PROT_READ
#define MAP_PRIVATE     AMIPORT_EMU_MAP_PRIVATE
/* amiport: madvise() stubbed -- no equivalent on AmigaOS */
#define madvise(a, l, f) ((void)0)
/* amiport: MADV_SEQUENTIAL referenced by stubbed madvise -- define to silence compiler */
#ifndef MADV_SEQUENTIAL
#define MADV_SEQUENTIAL 2
#endif

/*
 * amiport: SIZE_MAX not in libnix C89 headers.
 * off_t is long (32-bit) on AmigaOS, so SIZE_MAX = ULONG_MAX suffices.
 */
#ifndef SIZE_MAX
#define SIZE_MAX ((unsigned long)-1)
#endif

#define	MINIMUM(a, b)	(((a) < (b)) ? (a) : (b))

/*
 * amiport: signature gains path1/path2 so the mmap_failed fallback can
 * call c_special() with paths (c_special uses fopen(), not fdopen()).
 * Callers must pass the file path (or NULL for stdin) alongside the fd.
 */
void
c_regular(int fd1, const char *path1, char *file1, off_t skip1, off_t len1,
    int fd2, const char *path2, char *file2, off_t skip2, off_t len2)
{
	u_char ch, *p1, *p2;
	off_t byte, length, line;
	int dfound;

	if (skip1 > len1)
		eofmsg(file1);
	len1 -= skip1;
	if (skip2 > len2)
		eofmsg(file2);
	len2 -= skip2;

	/* amiport: exit(1)->exit(10) -- RETURN_ERROR; exit(1) invisible to AmigaDOS */
	if (sflag && len1 != len2)
		exit(DIFF_EXIT);

	length = MINIMUM(len1, len2);
	if (length > SIZE_MAX) {
	mmap_failed:
		/*
		 * amiport: close fds before c_special() reopens via fopen().
		 * c_special() takes paths (not fds) -- crash-patterns #12.
		 */
		if (path1 != NULL) amiport_close(fd1);
		if (path2 != NULL) amiport_close(fd2);
		c_special(path1, file1, skip1, path2, file2, skip2);
		return;
	}

	if ((p1 = mmap(NULL, (size_t)length, PROT_READ,
	    MAP_PRIVATE, fd1, skip1)) == MAP_FAILED)
		goto mmap_failed;
	if ((p2 = mmap(NULL, (size_t)length, PROT_READ,
	    MAP_PRIVATE, fd2, skip2)) == MAP_FAILED) {
		munmap(p1, (size_t)length);
		goto mmap_failed;
	}
	if (length) {
		madvise(p1, length, MADV_SEQUENTIAL);
		madvise(p2, length, MADV_SEQUENTIAL);
	}

	dfound = 0;
	for (byte = line = 1; length--; ++p1, ++p2, ++byte) {
		if ((ch = *p1) != *p2) {
			if (lflag) {
				dfound = 1;
				/* amiport: off_t is long (32-bit) on AmigaOS -- use %ld not %lld */
				(void)printf("%6ld %3o %3o\n", (long)byte,
				    ch, *p2);
			} else
				diffmsg(file1, file2, byte, line);
				/* NOTREACHED */
		}
		if (ch == '\n')
			++line;
	}

	if (len1 != len2)
		eofmsg (len1 > len2 ? file2 : file1);
	if (dfound)
		exit(DIFF_EXIT);
}
