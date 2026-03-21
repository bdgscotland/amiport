/*	$OpenBSD: file.c,v 1.17 2021/12/15 19:22:44 tb Exp $	*/
/* amiport: Ported to AmigaOS 3.x — major restructure:
 *   - Removed mmap support (no sys/mman.h on AmigaOS)
 *   - Removed gzip/zlib support (NOZ defined)
 *   - Replaced open()/fdopen() with fopen() (fdopen not in libnix)
 *   - Directory detection via opendir() instead of fstat()/S_ISDIR()
 */

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

/* amiport: replaced system includes */
/* amiport: removed <sys/stat.h>, <fcntl.h> — not needed (no open/fstat) */
/* amiport: removed <zlib.h> — no gzip support (NOZ) */
#include "grep.h"

static char	 fname[PATH_MAX];
static char	*lnbuf;
static size_t	 lnbufsize;

/* amiport: simplified — only FILE_STDIO mode (no mmap, no gzip) */
#define FILE_STDIO	0

struct file {
	int	 type;		/* amiport: always FILE_STDIO */
	int	 noseek;
	FILE	*f;
	/* amiport: removed mmf_t *mmf (no mmap) */
	/* amiport: removed gzFile gzf (no gzip) */
};

/* amiport: removed gzfgetln() — no gzip support */

file_t *
grep_fdopen(int fd)
{
	file_t *f;

	/* amiport: this is only called for stdin (STDIN_FILENO).
	 * Original code used fstat()+S_ISDIR()+fdopen() which aren't
	 * available in libnix. Use stdin FILE* directly instead. */
	(void)fd;

	if (fd == STDIN_FILENO)
		snprintf(fname, sizeof fname, "(standard input)");
	else
		snprintf(fname, sizeof fname, "(fd %d)", fd);

	f = grep_malloc(sizeof *f);
	f->type = FILE_STDIO;
	f->noseek = 1;  /* stdin is not seekable */
	f->f = stdin;
	return f;
}

file_t *
grep_open(char *path)
{
	file_t *f;
	DIR *d;

	snprintf(fname, sizeof fname, "%s", path);

	/* amiport: replaced fstat()/S_ISDIR() with opendir() check.
	 * fstat() not available in libnix; opendir uses AmigaDOS Lock(). */
	d = opendir(fname);
	if (d != NULL) {
		closedir(d);
		errno = EISDIR;
		return NULL;
	}

	f = grep_malloc(sizeof *f);
	f->type = FILE_STDIO;
	f->noseek = 0;

	/* amiport: replaced open()+fdopen() with fopen() directly.
	 * fdopen() not available in libnix -noixemul. */
	f->f = fopen(fname, "r");
	if (f->f == NULL) {
		free(f);
		return NULL;
	}
	return f;
}

int
grep_bin_file(file_t *f)
{
	if (f->noseek)
		return 0;

	/* amiport: only FILE_STDIO path — removed mmap and gzip paths */
	return bin_file(f->f);
}

char *
grep_fgetln(file_t *f, size_t *l)
{
	/* amiport: only FILE_STDIO path — removed mmap and gzip paths */
	if ((*l = getline(&lnbuf, &lnbufsize, f->f)) == (size_t)-1) {
		if (ferror(f->f))
			err(AMIGA_RETURN_ERROR, "%s: getline", fname);
		else
			return NULL;
	}
	return lnbuf;
}

void
grep_close(file_t *f)
{
	/* amiport: only FILE_STDIO path — removed mmap and gzip paths.
	 * Don't close stdin (it was not opened by us). */
	if (f->f != stdin)
		fclose(f->f);
	free(f);
}
