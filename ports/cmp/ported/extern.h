/*      $OpenBSD: extern.h,v 1.6 2018/03/05 16:53:39 cheloha Exp $      */
/*      $NetBSD: extern.h,v 1.2 1995/09/08 03:22:57 tls Exp $      */

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
 *
 *	@(#)extern.h	8.3 (Berkeley) 4/2/94
 */

/* amiport: DIFF_EXIT changed 1->10 -- exit(1) invisible to AmigaDOS scripts */
#define DIFF_EXIT	10
/* amiport: ERR_EXIT changed 2->10 -- RETURN_ERROR (AmigaDOS error convention) */
#define ERR_EXIT	10

/*
 * amiport: c_regular() gains path1/path2 parameters so the mmap_failed
 * fallback can call c_special() with paths (not fds).
 */
void	c_regular(int, const char *, char *, off_t, off_t,
	    int, const char *, char *, off_t, off_t);
/*
 * amiport: c_special() signature changed: fds replaced with file paths.
 * fdopen() on amiport_open() fds silently fails (crash-patterns #12).
 * path1/path2 == NULL means stdin -- c_special() uses fopen() or stdin FILE*.
 */
void	c_special(const char *, char *, off_t, const char *, char *, off_t);
void	diffmsg(char *, char *, off_t, off_t);
void	eofmsg(char *);
void	fatal(const char *, ...)
	    __attribute__((__noreturn__, __format__ (printf, 1, 2)));
void	fatalx(const char *, ...)
	    __attribute__((__noreturn__, __format__ (printf, 1, 2)));

extern int lflag, sflag;
