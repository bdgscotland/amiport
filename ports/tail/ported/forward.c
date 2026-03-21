/*	$OpenBSD: forward.c,v 1.33 2019/06/28 13:35:04 deraadt Exp $	*/
/*	$NetBSD: forward.c,v 1.7 1996/02/13 16:49:10 ghudson Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Sze-Tyan Wang.
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

/* amiport: removed #include <sys/types.h> — types provided by Amiga headers */
/* amiport: replaced #include <sys/stat.h> with amiport wrapper */
#include <amiport/sys/stat.h>
/* amiport: removed #include <sys/event.h> — kqueue not available on AmigaOS */
/* amiport-redesign: kqueue()/kevent() replaced with Delay() polling fallback — see below */

/* amiport: replaced #include <err.h> with amiport wrapper */
#include <amiport/err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* amiport: replaced #include <unistd.h> with amiport wrapper */
#include <amiport/unistd.h>
/* amiport: added signal.h for amiport_check_break() used in -f follow loop */
#include <amiport/signal.h>
/* amiport: added proto/dos.h for Delay() used in -f polling loop */
#include <proto/dos.h>

/* amiport: fseeko macro — off_t is 32-bit on AmigaOS, map to fseek */
#define fseeko(fp, off, w) fseek((fp), (long)(off), (w))

#include "extern.h"

static int rlines(struct tailfile *, off_t);
static inline void tfprint(FILE *fp);
/* amiport-redesign: tfqueue() and tfreopen() removed — kqueue not available on AmigaOS */

/* amiport-redesign: static int kq = -1 removed — kqueue not available on AmigaOS */

/*
 * forward -- display the file, from an offset, forward.
 *
 * There are eight separate cases for this -- regular and non-regular
 * files, by bytes or lines and from the beginning or end of the file.
 *
 * FBYTES	byte offset from the beginning of the file
 *	REG	seek
 *	NOREG	read, counting bytes
 *
 * FLINES	line offset from the beginning of the file
 *	REG	read, counting lines
 *	NOREG	read, counting lines
 *
 * RBYTES	byte offset from the end of the file
 *	REG	seek
 *	NOREG	cyclically read characters into a wrap-around buffer
 *
 * RLINES
 *	REG	step back until the correct offset is reached.
 *	NOREG	cyclically read lines into a wrap-around array of buffers
 */
void
forward(struct tailfile *tf, int nfiles, enum STYLE style, off_t origoff)
{
	int ch;
	int i;
	/* amiport-redesign: struct kevent ke removed — kqueue not available */
	/* amiport-redesign: const struct timespec *ts removed — not needed for polling */

	if (nfiles < 1)
		return;

	/* amiport-redesign: kqueue() initialisation removed — using Delay() polling instead */

	for (i = 0; i < nfiles; i++) {
		off_t off = origoff;
		if (nfiles > 1)
			printfname(tf[i].fname);

		switch(style) {
		case FBYTES:
			if (off == 0)
				break;
			if (S_ISREG(tf[i].sb.st_mode)) {
				if (tf[i].sb.st_size < off)
					off = tf[i].sb.st_size;
				if (fseeko(tf[i].fp, off, SEEK_SET) == -1) {
					ierr(tf[i].fname);
					return;
				}
			} else while (off--)
				if ((ch = getc(tf[i].fp)) == EOF) {
					if (ferror(tf[i].fp)) {
						ierr(tf[i].fname);
						return;
					}
					break;
				}
			break;
		case FLINES:
			if (off == 0)
				break;
			for (;;) {
				if ((ch = getc(tf[i].fp)) == EOF) {
					if (ferror(tf[i].fp)) {
						ierr(tf[i].fname);
						return;
					}
					break;
				}
				if (ch == '\n' && !--off)
					break;
			}
			break;
		case RBYTES:
			if (S_ISREG(tf[i].sb.st_mode)) {
				if (tf[i].sb.st_size >= off &&
				    fseeko(tf[i].fp, -off, SEEK_END) == -1) {
					ierr(tf[i].fname);
					return;
				}
			} else if (off == 0) {
				while (getc(tf[i].fp) != EOF)
					;
				if (ferror(tf[i].fp)) {
					ierr(tf[i].fname);
					return;
				}
			} else {
				if (bytes(&(tf[i]), off))
					return;
			}
			break;
		case RLINES:
			if (S_ISREG(tf[i].sb.st_mode)) {
				if (!off) {
					if (fseeko(tf[i].fp, (off_t)0,
					    SEEK_END) == -1) {
						ierr(tf[i].fname);
						return;
					}
				} else if (rlines(&(tf[i]), off) != 0)
					lines(&(tf[i]), off);
			} else if (off == 0) {
				while (getc(tf[i].fp) != EOF)
					;
				if (ferror(tf[i].fp)) {
					ierr(tf[i].fname);
					return;
				}
			} else {
				if (lines(&(tf[i]), off))
					return;
			}
			break;
		default:
			/* amiport: err(1,...) -> err(10,...) for AmigaOS RETURN_ERROR */
			err(10, "Unsupported style");
		}

		tfprint(tf[i].fp);
		/* amiport-redesign: tfqueue() call removed — polling loop handles follow below */
	}

	(void)fflush(stdout);

	/*
	 * amiport-redesign: kqueue not available on AmigaOS. Using Delay() polling
	 * fallback for -f (follow) mode. Polls all files every ~1 second (50 ticks).
	 * File deletion/rename/truncation detection (NOTE_DELETE, NOTE_RENAME,
	 * NOTE_TRUNCATE via EVFILT_VNODE) is not implemented — the original kqueue
	 * behaviour for those events is NOT replicated here.
	 * NEEDS HUMAN REVIEW if robust follow mode is required.
	 */
#ifdef __AMIGA__
	if (!fflag)
		return;

	{
		int last_displayed = -1;
		long curpos, endpos;
		int ch, has_data;

		while (1) {
			/* poll every ~1 second (50 ticks at 50Hz) */
			Delay(50);
			/* amiport: check for Ctrl-C break signal */
			if (amiport_check_break()) {
				(void)fflush(stdout);
				return;
			}
			for (i = 0; i < nfiles; i++) {
				clearerr(tf[i].fp);

				/* amiport: detect file truncation — if current position
				 * is past the new file size, the file was truncated.
				 * Rewind to the beginning like upstream kqueue handler. */
				curpos = ftell(tf[i].fp);
				if (curpos > 0) {
					fseek(tf[i].fp, 0L, SEEK_END);
					endpos = ftell(tf[i].fp);
					if (endpos < curpos) {
						fseek(tf[i].fp, 0L, SEEK_SET);
					} else {
						fseek(tf[i].fp, curpos, SEEK_SET);
					}
				}

				/* amiport: print filename header for multi-file follow */
				if (nfiles > 1) {
					has_data = 0;
					ch = fgetc(tf[i].fp);
					if (ch != EOF) {
						has_data = 1;
						ungetc(ch, tf[i].fp);
					}
					if (has_data && i != last_displayed) {
						printf("\n==> %s <==\n", tf[i].fname);
						last_displayed = i;
					}
				}

				tfprint(tf[i].fp);
				if (ferror(tf[i].fp)) {
					ierr(tf[i].fname);
				}
			}
			(void)fflush(stdout);
		}
	}
#endif /* __AMIGA__ */
}

/*
 * rlines -- display the last offset lines of the file.
 */
static int
rlines(struct tailfile *tf, off_t off)
{
	off_t pos;
	int ch;

	pos = tf->sb.st_size;
	if (pos == 0)
		return (0);

	/*
	 * Position before char.
	 * Last char is special, ignore it whether newline or not.
	 */
	pos -= 2;
	ch = EOF;
	for (; off > 0 && pos >= 0; pos--) {
		/* A seek per char isn't a problem with a smart stdio */
		if (fseeko(tf[0].fp, pos, SEEK_SET) == -1) {
			ierr(tf->fname);
			return (1);
		}
		if ((ch = getc(tf[0].fp)) == '\n')
			off--;
		else if (ch == EOF) {
			if (ferror(tf[0].fp)) {
				ierr(tf->fname);
				return (1);
			}
			break;
		}
	}
	/* If we read until start of file, put back last read char */
	if (pos < 0 && off > 0 && ch != EOF && ungetc(ch, tf[0].fp) == EOF) {
		ierr(tf->fname);
		return (1);
	}

	tfprint(tf[0].fp);
	if (ferror(tf[0].fp)) {
		ierr(tf->fname);
		return (1);
	}

	return (0);
}

static inline void
tfprint(FILE *fp)
{
	/* amiport: use fwrite(stdout) for AmigaDOS redirection compatibility.
	 * write(STDOUT_FILENO) bypasses AmigaDOS output redirection under ARexx/Execute.
	 * fwrite() uses the stdio layer which respects Output() handle. */
	char buf[4096];
	size_t n;

	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
		if (fwrite(buf, 1, n, stdout) != n)
			oerr();
	}
}

/* amiport-redesign: tfqueue() removed — used kqueue/kevent, not available on AmigaOS */
/* amiport-redesign: tfreopen() removed — used kqueue/kevent and EVFILT_VNODE, not available on AmigaOS */
