/*	$OpenBSD: comm.c,v 1.11 2022/12/04 23:50:47 cheloha Exp $	*/
/*	$NetBSD: comm.c,v 1.10 1995/09/05 19:57:43 jtc Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Case Larsen.
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

/* amiport: replaced <err.h> with <amiport/err.h> for AmigaOS-compatible err/errx */
#include <amiport/err.h>
#include <limits.h>
/* amiport: LINE_MAX is not defined by libnix limits.h; define a safe default */
#ifndef LINE_MAX
#define LINE_MAX 2048
#endif
#include <locale.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
#include <unistd.h>
/* amiport: needed for amiport_check_break() -- Ctrl-C handling on AmigaOS */
#include <amiport/signal.h>

/* amiport: pledge() not available on AmigaOS -- stubbed as no-op */
#define pledge(p, e) (0)

/* amiport: Amiga version string -- upstream OpenBSD comm v1.11 */
static const char *verstag = "$VER: comm 1.11 (24.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small for ported software */
long __stack = 32768;

#define	MAXLINELEN	(LINE_MAX + 1)

char *tabs[] = { "", "\t", "\t\t" };

FILE   *file(const char *);
void	show(FILE *, char *, char *);
void	usage(void);

int
main(int argc, char *argv[])
{
	int comp, file1done, file2done, read1, read2;
	int ch, flag1, flag2, flag3;
	FILE *fp1, *fp2;
	char *col1, *col2, *col3;
	char **p;
	/* amiport: static to keep ~4KB off the stack (known pitfall for 68k) */
	static char line1[MAXLINELEN], line2[MAXLINELEN];
	int (*compare)(const char * ,const char *);
	int break_counter = 0;

	setlocale(LC_ALL, "");

	/* amiport: pledge() stubbed -- no-op on AmigaOS */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	flag1 = flag2 = flag3 = 1;
	/* amiport: libnix has no real locale support; strcoll == strcmp on AmigaOS
	 * but with extra locale-dispatch overhead per call. Use strcmp directly. */
	compare = strcmp;
	while ((ch = getopt(argc, argv, "123f")) != -1)
		switch(ch) {
		case '1':
			flag1 = 0;
			break;
		case '2':
			flag2 = 0;
			break;
		case '3':
			flag3 = 0;
			break;
		case 'f':
			compare = strcasecmp;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	fp1 = file(argv[0]);
	fp2 = file(argv[1]);

	/* for each column printed, add another tab offset */
	p = tabs;
	col1 = col2 = col3 = NULL;
	if (flag1)
		col1 = *p++;
	if (flag2)
		col2 = *p++;
	if (flag3)
		col3 = *p;

	for (read1 = read2 = 1;;) {
		/* amiport: check for Ctrl-C every 64 iterations to reduce OS call overhead */
		if (++break_counter >= 64) {
			break_counter = 0;
			if (amiport_check_break()) {
				(void)fflush(stdout);
				break;
			}
		}
		/* read next line, check for EOF */
		if (read1)
			file1done = !fgets(line1, MAXLINELEN, fp1);
		if (read2)
			file2done = !fgets(line2, MAXLINELEN, fp2);

		/* if one file done, display the rest of the other file */
		if (file1done) {
			if (!file2done && col2)
				show(fp2, col2, line2);
			break;
		}
		if (file2done) {
			if (!file1done && col1)
				show(fp1, col1, line1);
			break;
		}

		/* lines are the same */
		if (!(comp = compare(line1, line2))) {
			read1 = read2 = 1;
			/* amiport: fputs avoids printf format dispatch overhead on 68k */
			if (col3)
				if (fputs(col3, stdout) == EOF || fputs(line1, stdout) == EOF)
					break;
			continue;
		}

		/* lines are different */
		if (comp < 0) {
			read1 = 1;
			read2 = 0;
			if (col1)
				if (fputs(col1, stdout) == EOF || fputs(line1, stdout) == EOF)
					break;
		} else {
			read1 = 0;
			read2 = 1;
			if (col2)
				if (fputs(col2, stdout) == EOF || fputs(line2, stdout) == EOF)
					break;
		}
	}

	/* amiport: replaced fclose(stdout) with fflush(stdout) -- fclose() would
	 * close the shell's Output() handle, which is dangerous on Workbench */
	(void)fflush(stdout);
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */

	exit(0);
}

void
show(FILE *fp, char *offset, char *buf)
{
	int brk = 0;
	do {
		/* amiport: check for Ctrl-C every 64 iterations */
		if (++brk >= 64) {
			brk = 0;
			if (amiport_check_break()) {
				(void)fflush(stdout);
				return;
			}
		}
		/* amiport: fputs avoids printf format dispatch overhead on 68k */
		if (fputs(offset, stdout) == EOF || fputs(buf, stdout) == EOF)
			break;
	} while (fgets(buf, MAXLINELEN, fp));
}

FILE *
file(const char *name)
{
	FILE *fp;

	if (!strcmp(name, "-"))
		return (stdin);
	if ((fp = fopen(name, "r")) == NULL)
		err(10, "%s", name); /* amiport: RETURN_ERROR */
	return (fp);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: comm [-123f] file1 file2\n");
	exit(10); /* amiport: RETURN_ERROR -- visible to Amiga IF ERROR scripts */
}
