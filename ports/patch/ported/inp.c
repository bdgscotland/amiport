/*	$OpenBSD: inp.c,v 1.49 2019/06/28 13:35:02 deraadt Exp $	*/

/*
 * patch - a program to apply diffs to original files
 * 
 * Copyright 1986, Larry Wall
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this condition and the following disclaimer.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * -C option added in 1998, original code by Marc Espie, based on FreeBSD
 * behaviour
 */

/* amiport: replaced <sys/mman.h> with amiport-emu mmap emulation */
#include <amiport-emu/mmap.h>
/* amiport-emu: mmap() emulated via AllocMem+Read — entire file loaded upfront,
 * no lazy paging, read-only MAP_PRIVATE only. plan_b fallback is available. */

/* amiport: stat shim — must come before <fcntl.h> which pulls in system sys/stat.h.
 * Define AMIPORT_NO_STAT_MACROS so amiport/sys/stat.h does not redefine S_IF*
 * constants already present in the system headers. We call amiport_stat() directly. */
#define AMIPORT_NO_STAT_MACROS
#include <amiport/sys/stat.h>

#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <limits.h>
/* amiport: SIZE_MAX not in libnix limits.h — define from size_t */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with amiport unistd shim */
#include <amiport/unistd.h>
/* amiport: for reallocarray */
#include <amiport/string.h>

/* amiport: madvise() not available on AmigaOS — no-op */
#define madvise(p, l, a) ((void)0)

/* amiport: EXDEV may not be defined in AmigaOS errno.h — provide fallback */
#ifndef EXDEV
#define EXDEV 18
#endif

#include "common.h"
#include "util.h"
#include "pch.h"
#include "inp.h"


/* Input-file-with-indexable-lines abstract type */

static off_t	i_size;		/* size of the input file */
static char	*i_womp;	/* plan a buffer for entire file */
static char	**i_ptr;	/* pointers to lines in i_womp */

static int	tifd = -1;	/* plan b virtual string array */
static char	*tibuf[2];	/* plan b buffers */
static LINENUM	tiline[2] = {-1, -1};	/* 1st line in each buffer */
static size_t	lines_per_buf;	/* how many lines per buffer */
static size_t	tibuflen;	/* plan b buffer length */
static size_t	tireclen;	/* length of records in tmp file */

static bool	rev_in_string(const char *);
static bool	reallocate_lines(size_t *);

/* returns false if insufficient memory */
static bool	plan_a(const char *);

static void	plan_b(const char *);

/* New patch--prepare to edit another file. */

void
re_input(void)
{
	if (using_plan_a) {
		free(i_ptr);
		i_ptr = NULL;
		if (i_womp != NULL) {
			amiport_emu_munmap(i_womp, i_size); /* amiport-emu: replaced munmap() */
			i_womp = NULL;
		}
		i_size = 0;
	} else {
		using_plan_a = true;	/* maybe the next one is smaller */
		amiport_close(tifd); /* amiport: replaced close() */
		tifd = -1;
		free(tibuf[0]);
		free(tibuf[1]);
		tibuf[0] = tibuf[1] = NULL;
		tiline[0] = tiline[1] = -1;
		tireclen = 0;
	}
}

/* Construct the line index, somehow or other. */

void
scan_input(const char *filename)
{
	if (!plan_a(filename))
		plan_b(filename);
	if (verbose) {
		say("Patching file %s using Plan %s...\n", filename,
		    (using_plan_a ? "A" : "B"));
	}
}

static bool
reallocate_lines(size_t *lines_allocatedp)
{
	char	**p;
	size_t	new_size;

	new_size = *lines_allocatedp * 3 / 2;
	p = reallocarray(i_ptr, new_size + 2, sizeof(char *));
	if (p == NULL) {	/* shucks, it was a near thing */
		amiport_emu_munmap(i_womp, i_size); /* amiport-emu: replaced munmap() */
		i_womp = NULL;
		free(i_ptr);
		i_ptr = NULL;
		*lines_allocatedp = 0;
		return false;
	}
	*lines_allocatedp = new_size;
	i_ptr = p;
	return true;
}

/* Try keeping everything in memory. */

static bool
plan_a(const char *filename)
{
	int		ifd, statfailed;
	char		*p, *s;
	struct amiport_stat	filestat; /* amiport: explicit type — AMIPORT_NO_STAT_MACROS suppresses typedef */
	off_t		i;
	ptrdiff_t	sz;
	size_t		iline, lines_allocated;

#ifdef DEBUGGING
	if (debug & 8)
		return false;
#endif

	if (filename == NULL || *filename == '\0')
		return false;

	statfailed = amiport_stat(filename, &filestat); /* amiport: replaced stat() */
	if (statfailed && ok_to_create_file) {
		int fd;

		if (verbose)
			say("(Creating file %s...)\n", filename);

		/*
		 * in check_patch case, we still display `Creating file' even
		 * though we're not. The rule is that -C should be as similar
		 * to normal patch behavior as possible
		 */
		if (check_only)
			return true;
		makedirs(filename, true);
		/* amiport: replaced open()+close() for file creation */
		if ((fd = amiport_open(filename, O_CREAT | O_TRUNC | O_WRONLY)) != -1)
			amiport_close(fd);

		statfailed = amiport_stat(filename, &filestat); /* amiport: replaced stat() */
	}
	if (statfailed)
		fatal("can't find %s\n", filename);
	filemode = filestat.st_mode;
	if (!S_ISREG(filemode))
		fatal("%s is not a normal file--can't patch\n", filename);
	i_size = filestat.st_size;
	if (out_of_mem) {
		set_hunkmax();	/* make sure dynamic arrays are allocated */
		out_of_mem = false;
		return false;	/* force plan b because plan a bombed */
	}
	if (i_size > SIZE_MAX) {
		say("block too large to mmap\n");
		return false;
	}
	/* amiport: replaced open() with amiport_open() */
	if ((ifd = amiport_open(filename, O_RDONLY)) == -1)
		pfatal("can't open file %s", filename);

	if (i_size) {
		/* amiport-emu: replaced mmap() — AllocMem+Read, no lazy paging */
		i_womp = amiport_emu_mmap(NULL, i_size, AMIPORT_EMU_PROT_READ, AMIPORT_EMU_MAP_PRIVATE, ifd, 0);
		if (i_womp == AMIPORT_EMU_MAP_FAILED) {
			perror("mmap failed");
			i_womp = NULL;
			amiport_close(ifd); /* amiport: replaced close() */
			return false;
		}
	} else {
		i_womp = NULL;
	}

	amiport_close(ifd); /* amiport: replaced close() */
	if (i_size)
		madvise(i_womp, i_size, MADV_SEQUENTIAL);

	/* estimate the number of lines */
	lines_allocated = i_size / 25;
	if (lines_allocated < 100)
		lines_allocated = 100;

	if (!reallocate_lines(&lines_allocated))
		return false;

	/* now scan the buffer and build pointer array */
	iline = 1;
	i_ptr[iline] = i_womp;
	/* test for NUL too, to maintain the behavior of the original code */
	for (s = i_womp, i = 0; i < i_size && *s != '\0'; s++, i++) {
		if (*s == '\n') {
			if (iline == lines_allocated) {
				if (!reallocate_lines(&lines_allocated))
					return false;
			}
			/* these are NOT NUL terminated */
			i_ptr[++iline] = s + 1;
		}
	}
	/* if the last line contains no EOL, append one */
	if (i_size > 0 && i_womp[i_size - 1] != '\n') {
		last_line_missing_eol = true;
		/* fix last line */
		sz = s - i_ptr[iline];
		p = malloc(sz + 1);
		if (p == NULL) {
			free(i_ptr);
			i_ptr = NULL;
			amiport_emu_munmap(i_womp, i_size); /* amiport-emu: replaced munmap() */
			i_womp = NULL;
			return false;
		}

		memcpy(p, i_ptr[iline], sz);
		p[sz] = '\n';
		i_ptr[iline] = p;
		/* count the extra line and make it point to some valid mem */
		i_ptr[++iline] = "";
	} else
		last_line_missing_eol = false;

	input_lines = iline - 1;

	/* now check for revision, if any */

	if (revision != NULL) {
		if (i_womp == NULL || !rev_in_string(i_womp)) {
			if (force) {
				if (verbose)
					say("Warning: this file doesn't appear "
					    "to be the %s version--patching anyway.\n",
					    revision);
			} else if (batch) {
				fatal("this file doesn't appear to be the "
				    "%s version--aborting.\n",
				    revision);
			} else {
				ask("This file doesn't appear to be the "
				    "%s version--patch anyway? [n] ",
				    revision);
				if (*buf != 'y')
					fatal("aborted\n");
			}
		} else if (verbose)
			say("Good.  This file appears to be the %s version.\n",
			    revision);
	}
	return true;		/* plan a will work */
}

/* Keep (virtually) nothing in memory. */

static void
plan_b(const char *filename)
{
	FILE	*ifp;
	size_t	i = 0, j, len, maxlen = 1;
	char	*lbuf = NULL, *p;
	bool	found_revision = (revision == NULL);

	using_plan_a = false;
	if ((ifp = fopen(filename, "r")) == NULL)
		pfatal("can't open file %s", filename);
	(void) amiport_unlink(TMPINNAME); /* amiport: replaced unlink() */
	/* amiport: replaced open() with amiport_open() */
	if ((tifd = amiport_open(TMPINNAME, O_EXCL | O_CREAT | O_WRONLY)) == -1)
		pfatal("can't open file %s", TMPINNAME);
	/* amiport: replaced fgetln() with fgets() — fgetln not available on AmigaOS.
	 * Using static buffer; MAXHUNKSIZE+2 covers the largest possible line.
	 * Static so the large buffer does not consume stack (crash-patterns #10). */
	{
	static char fgetln_buf[MAXHUNKSIZE + 2];
	while (fgets(fgetln_buf, sizeof(fgetln_buf), ifp) != NULL) {
		p = fgetln_buf;
		len = strlen(p);
		if (len > 0 && p[len - 1] == '\n') {
			p[len - 1] = '\0';
		} else {
			/* EOF without EOL — copy to lbuf and add NUL */
			/* amiport: len bounded by sizeof(fgetln_buf)=MAXHUNKSIZE+2 (semgrep false-positive) */
			if ((lbuf = malloc(len + 1)) == NULL)
				fatal("out of memory\n");
			memcpy(lbuf, p, len);
			lbuf[len] = '\0';
			p = lbuf;

			last_line_missing_eol = true;
			len++;
		}
		if (revision != NULL && !found_revision && rev_in_string(p))
			found_revision = true;
		if (len > maxlen)
			maxlen = len;	/* find longest line */
	}
	} /* end fgets loop / fgetln_buf scope */
	free(lbuf);
	if (ferror(ifp))
		pfatal("can't read file %s", filename);

	if (revision != NULL) {
		if (!found_revision) {
			if (force) {
				if (verbose)
					say("Warning: this file doesn't appear "
					    "to be the %s version--patching anyway.\n",
					    revision);
			} else if (batch) {
				fatal("this file doesn't appear to be the "
				    "%s version--aborting.\n",
				    revision);
			} else {
				ask("This file doesn't appear to be the %s "
				    "version--patch anyway? [n] ",
				    revision);
				if (*buf != 'y')
					fatal("aborted\n");
			}
		} else if (verbose)
			say("Good.  This file appears to be the %s version.\n",
			    revision);
	}
	fseek(ifp, 0L, SEEK_SET);	/* rewind file */
	tireclen = maxlen;
	tibuflen = maxlen > BUFFERSIZE ? maxlen : BUFFERSIZE;
	lines_per_buf = tibuflen / maxlen;
	/* amiport: tibuflen = max(maxlen, BUFFERSIZE); maxlen <= MAXHUNKSIZE+2 (semgrep false-positive) */
	tibuf[0] = malloc(tibuflen + 1);
	if (tibuf[0] == NULL)
		fatal("out of memory\n");
	tibuf[1] = malloc(tibuflen + 1);
	if (tibuf[1] == NULL)
		fatal("out of memory\n");
	for (i = 1;; i++) {
		p = tibuf[0] + maxlen * (i % lines_per_buf);
		if (i % lines_per_buf == 0)	/* new block */
			/* amiport: replaced write() with amiport_write() */
			if (amiport_write(tifd, tibuf[0], tibuflen) !=
			    (LONG) tibuflen)
				pfatal("can't write temp file");
		if (fgets(p, maxlen + 1, ifp) == NULL) {
			input_lines = i - 1;
			if (i % lines_per_buf != 0)
				/* amiport: replaced write() with amiport_write() */
				if (amiport_write(tifd, tibuf[0], tibuflen) !=
				    (LONG) tibuflen)
					pfatal("can't write temp file");
			break;
		}
		j = strlen(p);
		/* These are '\n' terminated strings, so no need to add a NUL */
		if (j == 0 || p[j - 1] != '\n')
			p[j] = '\n';
	}
	fclose(ifp);
	amiport_close(tifd); /* amiport: replaced close() */
	/* amiport: replaced open() with amiport_open() */
	if ((tifd = amiport_open(TMPINNAME, O_RDONLY)) == -1)
		pfatal("can't reopen file %s", TMPINNAME);
}

/*
 * Fetch a line from the input file, \n terminated, not necessarily \0.
 */
char *
ifetch(LINENUM line, int whichbuf)
{
	if (line < 1 || line > input_lines) {
		if (warn_on_invalid_line) {
			say("No such line %ld in input file, ignoring\n", line);
			warn_on_invalid_line = false;
		}
		return NULL;
	}
	if (using_plan_a)
		return i_ptr[line];
	else {
		LINENUM	offline = line % lines_per_buf;
		LINENUM	baseline = line - offline;

		if (tiline[0] == baseline)
			whichbuf = 0;
		else if (tiline[1] == baseline)
			whichbuf = 1;
		else {
			tiline[whichbuf] = baseline;

			/* amiport: replaced lseek() with amiport_lseek() */
			if (amiport_lseek(tifd, (LONG) (baseline / lines_per_buf *
			    tibuflen), SEEK_SET) == -1)
				pfatal("cannot seek in the temporary input file");

			/* amiport: replaced read() with amiport_read() */
			if (amiport_read(tifd, tibuf[whichbuf], tibuflen)
			    != (LONG) tibuflen)
				pfatal("error reading tmp file %s", TMPINNAME);
		}
		return tibuf[whichbuf] + (tireclen * offline);
	}
}

/*
 * True if the string argument contains the revision number we want.
 */
static bool
rev_in_string(const char *string)
{
	const char	*s;
	size_t		patlen;

	if (revision == NULL)
		return true;
	patlen = strlen(revision);
	if (strnEQ(string, revision, patlen) &&
	    isspace((unsigned char)string[patlen]))
		return true;
	for (s = string; *s; s++) {
		if (isspace((unsigned char)*s) && strnEQ(s + 1, revision, patlen) &&
		    isspace((unsigned char)s[patlen + 1])) {
			return true;
		}
	}
	return false;
}
