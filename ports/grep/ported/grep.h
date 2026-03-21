/*	$OpenBSD: grep.h,v 1.29 2022/06/26 10:57:36 op Exp $	*/
/* amiport: Ported to AmigaOS 3.x — removed zlib and mmap dependencies */

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coidan Smorgrav
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef GREP_H
#define GREP_H

/* amiport: replaced system includes with compat.h */
#include "compat.h"

/* amiport: removed <sys/types.h>, <sys/stat.h> — provided by compat.h */
/* amiport: removed <regex.h> — using Tier 2 emulation from compat.h */
/* amiport: removed <zlib.h> — building without gzip support (NOZ) */

#include <stdint.h>

#define VER_MAJ 0
#define VER_MIN 9

#define BIN_FILE_BIN	0
#define BIN_FILE_SKIP	1
#define BIN_FILE_TEXT	2

typedef struct {
	size_t		 len;
	long long	 line_no;
	off_t		 off;
	char		*file;
	char		*dat;
} str_t;

typedef struct {
	unsigned char	*pattern;
	int		 patternLen;
	int		 qsBc[UCHAR_MAX + 1];
	/* flags */
	int		 bol;
	int		 eol;
	int		 wmatch;
	int		 reversedSearch;
} fastgrep_t;

/* Flags passed to regcomp() and regexec() */
extern int	 cflags, eflags;

/* Command line flags */
/* amiport: removed Zflag — no gzip support */
extern int	 Aflag, Bflag, Eflag, Fflag, Hflag, Lflag,
		 Rflag,
		 bflag, cflag, hflag, iflag, lflag, mflag, nflag, oflag, qflag,
		 sflag, vflag, wflag, xflag, nullflag;
extern int	 binbehave;
extern const char *labelname;

extern int	 first, matchall, patterns, tail, file_err;
extern char    **pattern;
extern fastgrep_t *fg_pattern;
extern regex_t	*r_pattern;

/* For -m max-count */
extern long long mcount, mlimit;

/* For regex errors  */
#define RE_ERROR_BUF 512
extern char	 re_error[RE_ERROR_BUF + 1];

/* util.c */
int		 procfile(char *fn);
int		 grep_tree(char **argv);
void		*grep_malloc(size_t size);
void		*grep_calloc(size_t nmemb, size_t size);
void		*grep_realloc(void *ptr, size_t size);
void		*grep_reallocarray(void *ptr, size_t nmemb, size_t size);
void		 printline(str_t *line, int sep, regmatch_t *pmatch);
int		 fastcomp(fastgrep_t *, const char *);
void		 fgrepcomp(fastgrep_t *, const unsigned char *);

/* queue.c */
void		 initqueue(void);
void		 enqueue(str_t *x);
void		 printqueue(void);
void		 clearqueue(void);

/* amiport: removed mmfile.c declarations — no mmap support on AmigaOS */

/* file.c */
struct file;
typedef struct file file_t;

file_t		*grep_fdopen(int fd);
file_t		*grep_open(char *path);
int		 grep_bin_file(file_t *f);
char		*grep_fgetln(file_t *f, size_t *l);
void		 grep_close(file_t *f);
/* amiport: memory leak fix — free static lnbuf before exit */
void		 grep_free_lnbuf(void);

/* binary.c */
int		 bin_file(FILE * f);
/* amiport: removed gzbin_file and mmbin_file — no gzip/mmap support */

#endif /* GREP_H */
