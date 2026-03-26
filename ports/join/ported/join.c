/* $OpenBSD: join.c,v 1.34 2022/12/04 23:50:48 cheloha Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Steve Hayman of the Computer Science Department, Indiana University,
 * Michiro Hikida and David Goodenough.
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

/* amiport: AmigaOS 3.x port of OpenBSD join 1.34 */
/* amiport: $VER string for version command */
static const char *verstag = "$VER: join 1.34 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: removed <err.h> -- err/errx/warn/warnx provided by <amiport/err.h> below */
#include <errno.h>
#include <limits.h>
/* amiport: removed <locale.h> -- setlocale() stubbed via pledge macro below */
#include <stdio.h>
#include <amiport/stdlib.h>   /* amiport: replaced <stdlib.h> -- activates exit() -> amiport_exit() macro */
#include <string.h>
#include <amiport/unistd.h>   /* amiport: replaced <unistd.h> */
/* amiport: removed <wchar.h> -- no wchar support on AmigaOS 3.x; multibyte paths guarded below */

#include <amiport/glob.h>     /* amiport: provides amiport_expand_argv(), amiport_free_argv(), __progname */
#include <amiport/string.h>   /* amiport: provides amiport_reallocarray() via reallocarray macro */
#include <amiport/err.h>      /* amiport: provides err(), errx(), warn(), warnx() */
#include <amiport/getopt.h>   /* amiport: provides getopt(), optarg, optind -- libnix getopt broken */
#include <amiport/stdio_ext.h> /* amiport: provides getline() */

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: setlocale already provided by <amiport/unistd.h> -- no local stub needed */

/*
 * There's a structure per input file which encapsulates the state of the
 * file.  We repeatedly read lines from each file until we've read in all
 * the consecutive lines from the file with a common join field.  Then we
 * compare the set of lines with an equivalent set from the other file.
 */
typedef struct {
	char *line;			/* line */
	size_t linealloc;		/* bytes allocated for line */
	char **fields;			/* line field(s) */
	u_long fieldcnt;		/* line field(s) count */
	u_long fieldalloc;		/* line field(s) allocated count */
} LINE;

typedef struct {
	FILE *fp;			/* file descriptor */
	u_long joinf;			/* join field (-1, -2, -j) */
	int unpair;			/* output unpairable lines (-a) */
	u_long number;			/* 1 for file 1, 2 for file 2 */
	LINE *set;			/* set of lines with same field */
	int pushbool;			/* if pushback is set */
	u_long pushback;		/* line on the stack */
	u_long setcnt;			/* set count */
	u_long setalloc;		/* set allocated count */
} INPUT;
INPUT input1 = { NULL, 0, 0, 1, NULL, 0, 0, 0, 0 },
      input2 = { NULL, 0, 0, 2, NULL, 0, 0, 0, 0 };

typedef struct {
	u_long	filenum;	/* file number */
	u_long	fieldno;	/* field number */
} OLIST;
OLIST *olist;			/* output field list */
u_long olistcnt;		/* output field list count */
u_long olistalloc;		/* output field allocated count */

int joinout = 1;		/* show lines with matched join fields (-v) */
int needsep;			/* need separator character */
int spans = 1;			/* span multiple delimiters (-t) */
char *empty;			/* empty field replacement string (-e) */
/*
 * amiport: replaced wchar_t tabchar[] with char tabchar[] -- AmigaOS is ASCII-only,
 * no wchar/multibyte support (wchar.h removed). Delimiter logic simplified to
 * single-byte ASCII characters. tabchar[0] is the active delimiter,
 * tabchar[1] is the secondary (space+tab when spans=1).
 */
char tabchar[] = " \t";	/* delimiter characters (-t) */

int  cmp(LINE *, u_long, LINE *, u_long);
void fieldarg(char *);
void joinlines(INPUT *, INPUT *);
/*
 * amiport: replaced wchar_t* parameter in mbssep() with char* -- wchar removed.
 * On ASCII-only AmigaOS the delimiters are always single-byte characters.
 */
char *mbssep(char **, const char *);
void obsolete(char **);
void outfield(LINE *, u_long, int);
void outoneline(INPUT *, LINE *);
void outtwoline(INPUT *, LINE *, INPUT *, LINE *);
void slurp(INPUT *);
void usage(void);

/* amiport: track obsolete() malloc'd strings for cleanup */
#define MAX_OBSOLETE_ALLOCS 8
static char *obsolete_allocs[MAX_OBSOLETE_ALLOCS];
static int obsolete_alloc_count;

/* amiport: atexit cleanup -- frees expanded argv, obsolete allocs, flushes stdout */
static void
cleanup(void)
{
	int i;
	for (i = 0; i < obsolete_alloc_count; i++)
		free(obsolete_allocs[i]);
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	INPUT *F1, *F2;
	int aflag, ch, cval, vflag;
	char *end;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	setlocale(LC_CTYPE, "");  /* amiport: no-op on AmigaOS (stubbed above) */

	if (pledge("stdio rpath", NULL) == -1)  /* amiport: pledge stubbed -- always returns 0 */
		err(10, "pledge");

	F1 = &input1;
	F2 = &input2;

	aflag = vflag = 0;
	obsolete(argv);
	while ((ch = getopt(argc, argv, "\01a:e:j:1:2:o:t:v:")) != -1) {
		switch (ch) {
		case '\01':		/* See comment in obsolete(). */
			aflag = 1;
			F1->unpair = F2->unpair = 1;
			break;
		case '1':
			if ((F1->joinf = strtol(optarg, &end, 10)) < 1)
				errx(10, "-1 option field number less than 1"); /* amiport: RETURN_ERROR */
			if (*end)
				errx(10, "illegal field number -- %s", optarg); /* amiport: RETURN_ERROR */
			--F1->joinf;
			break;
		case '2':
			if ((F2->joinf = strtol(optarg, &end, 10)) < 1)
				errx(10, "-2 option field number less than 1"); /* amiport: RETURN_ERROR */
			if (*end)
				errx(10, "illegal field number -- %s", optarg); /* amiport: RETURN_ERROR */
			--F2->joinf;
			break;
		case 'a':
			aflag = 1;
			switch(strtol(optarg, &end, 10)) {
			case 1:
				F1->unpair = 1;
				break;
			case 2:
				F2->unpair = 1;
				break;
			default:
				errx(10, "-a option file number not 1 or 2"); /* amiport: RETURN_ERROR */
				break;
			}
			if (*end)
				errx(10, "illegal file number -- %s", optarg); /* amiport: RETURN_ERROR */
			break;
		case 'e':
			empty = optarg;
			break;
		case 'j':
			if ((F1->joinf = F2->joinf = strtol(optarg, &end, 10)) < 1)
				errx(10, "-j option field number less than 1"); /* amiport: RETURN_ERROR */
			if (*end)
				errx(10, "illegal field number -- %s", optarg); /* amiport: RETURN_ERROR */
			--F1->joinf;
			--F2->joinf;
			break;
		case 'o':
			fieldarg(optarg);
			break;
		case 't':
			spans = 0;
			/*
			 * amiport: replaced mbtowc(tabchar, optarg, MB_CUR_MAX) != strlen(optarg)
			 * with strlen(optarg) != 1 -- AmigaOS is ASCII-only, tab char must be
			 * a single ASCII byte. MB_CUR_MAX guarded: could return >1 on libnix
			 * (crash-patterns #11). wchar/multibyte path removed entirely.
			 */
			if (strlen(optarg) != 1)
				errx(10, "illegal tab character specification"); /* amiport: RETURN_ERROR */
			tabchar[0] = optarg[0];
			tabchar[1] = '\0';
			break;
		case 'v':
			vflag = 1;
			joinout = 0;
			switch (strtol(optarg, &end, 10)) {
			case 1:
				F1->unpair = 1;
				break;
			case 2:
				F2->unpair = 1;
				break;
			default:
				errx(10, "-v option file number not 1 or 2"); /* amiport: RETURN_ERROR */
				break;
			}
			if (*end)
				errx(10, "illegal file number -- %s", optarg); /* amiport: RETURN_ERROR */
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (aflag && vflag)
		errx(10, "the -a and -v options are mutually exclusive"); /* amiport: RETURN_ERROR */

	if (argc != 2)
		usage();

	/* Open the files; "-" means stdin. */
	if (!strcmp(*argv, "-"))
		F1->fp = stdin;
	else if ((F1->fp = fopen(*argv, "r")) == NULL)
		err(10, "%s", *argv); /* amiport: RETURN_ERROR */
	++argv;
	if (!strcmp(*argv, "-"))
		F2->fp = stdin;
	else if ((F2->fp = fopen(*argv, "r")) == NULL)
		err(10, "%s", *argv); /* amiport: RETURN_ERROR */
	if (F1->fp == stdin && F2->fp == stdin)
		errx(10, "only one input file may be stdin"); /* amiport: RETURN_ERROR */

	if (pledge("stdio", NULL) == -1)  /* amiport: pledge stubbed -- always returns 0 */
		err(10, "pledge");

	slurp(F1);
	slurp(F2);

	/*
	 * We try to let the files have the same field value, advancing
	 * whoever falls behind and always advancing the file(s) we output
	 * from.
	*/
	while (F1->setcnt && F2->setcnt) {
		cval = cmp(F1->set, F1->joinf, F2->set, F2->joinf);
		if (cval == 0) {
			/* Oh joy, oh rapture, oh beauty divine! */
			if (joinout)
				joinlines(F1, F2);
			slurp(F1);
			slurp(F2);
		} else if (cval < 0) {
			/* File 1 takes the lead... */
			if (F1->unpair)
				joinlines(F1, NULL);
			slurp(F1);
		} else {
			/* File 2 takes the lead... */
			if (F2->unpair)
				joinlines(F2, NULL);
			slurp(F2);
		}
	}

	/*
	 * Now that one of the files is used up, optionally output any
	 * remaining lines from the other file.
	 */
	if (F1->unpair)
		while (F1->setcnt) {
			joinlines(F1, NULL);
			slurp(F1);
		}
	if (F2->unpair)
		while (F2->setcnt) {
			joinlines(F2, NULL);
			slurp(F2);
		}

	return 0;
}

void
slurp(INPUT *F)
{
	LINE *lp, *lastlp, tmp;
	long len;  /* amiport: replaced ssize_t with long -- ssize_t is POSIX, use long on AmigaOS */
	u_long cnt;
	char *bp, *fieldp;

	/*
	 * Read all of the lines from an input file that have the same
	 * join field.
	 */

	F->setcnt = 0;
	for (lastlp = NULL; ; ++F->setcnt) {
		/*
		 * If we're out of space to hold line structures, allocate
		 * more.  Initialize the structure so that we know that this
		 * is new space.
		 */
		if (F->setcnt == F->setalloc) {
			LINE *p;
			u_long newsize = F->setalloc + 50;
			cnt = F->setalloc;
			/* amiport: reallocarray() provided by <amiport/string.h> macro */
			if ((p = reallocarray(F->set, newsize, sizeof(LINE)))
			    == NULL)
				err(10, NULL); /* amiport: RETURN_ERROR */
			F->set = p;
			F->setalloc = newsize;
			memset(F->set + cnt, 0, 50 * sizeof(LINE));
			/* re-set lastlp in case it moved */
			if (lastlp != NULL)
				lastlp = &F->set[F->setcnt - 1];
		}
		/*
		 * Get any pushed back line, else get the next line.  Allocate
		 * space as necessary.  If taking the line from the stack swap
		 * the two structures so that we don't lose space allocated to
		 * either structure.  This could be avoided by doing another
		 * level of indirection, but it's probably okay as is.
		 */
		lp = &F->set[F->setcnt];
		if (F->setcnt)
			lastlp = &F->set[F->setcnt - 1];
		if (F->pushbool) {
			tmp = F->set[F->setcnt];
			F->set[F->setcnt] = F->set[F->pushback];
			F->set[F->pushback] = tmp;
			F->pushbool = 0;
			continue;
		}
		if ((len = getline(&(lp->line), &(lp->linealloc), F->fp)) == -1)
			break;

		/* Remove the trailing newline, if any. */
		if (lp->line[len - 1] == '\n')
			lp->line[--len] = '\0';

		/* Split the line into fields, allocate space as necessary. */
		lp->fieldcnt = 0;
		bp = lp->line;
		while ((fieldp = mbssep(&bp, tabchar)) != NULL) {
			if (spans && *fieldp == '\0')
				continue;
			if (lp->fieldcnt == lp->fieldalloc) {
				char **p;
				u_long newsize = lp->fieldalloc + 50;
				/* amiport: reallocarray() provided by <amiport/string.h> macro */
				if ((p = reallocarray(lp->fields, newsize,
				    sizeof(char *))) == NULL)
					err(10, NULL); /* amiport: RETURN_ERROR */
				lp->fields = p;
				lp->fieldalloc = newsize;
			}
			lp->fields[lp->fieldcnt++] = fieldp;
		}

		/* See if the join field value has changed. */
		if (lastlp != NULL && cmp(lp, F->joinf, lastlp, F->joinf)) {
			F->pushbool = 1;
			F->pushback = F->setcnt;
			break;
		}
	}
}

/*
 * amiport: mbssep() rewritten for ASCII-only AmigaOS.
 * Original used wchar_t* delimiters, wctomb(), wcslen() for multibyte support.
 * On AmigaOS (no wchar.h, ASCII-only), delimiters are always single-byte ASCII.
 * tabchar[] is now char[] with at most 2 delimiter bytes.
 * The logic is functionally identical for single-byte delimiters.
 */
char *
mbssep(char **stringp, const char *delim)
{
	char *s, *p;
	int i;
	int ndelim;

	if ((s = *stringp) == NULL)
		return NULL;

	/* count non-NUL delimiter characters (tabchar is at most 2 bytes) */
	ndelim = 0;
	while (delim[ndelim] != '\0')
		ndelim++;

	for (p = s; *p != '\0'; p++) {
		for (i = 0; i < ndelim; i++) {
			if (*p == delim[i]) {
				*p = '\0';
				*stringp = p + 1;
				return s;
			}
		}
	}
	*stringp = NULL;
	return s;
}

int
cmp(LINE *lp1, u_long fieldno1, LINE *lp2, u_long fieldno2)
{
	if (lp1->fieldcnt <= fieldno1)
		return lp2->fieldcnt <= fieldno2 ? 0 : -1;
	if (lp2->fieldcnt <= fieldno2)
		return 1;
	return strcmp(lp1->fields[fieldno1], lp2->fields[fieldno2]);
}

void
joinlines(INPUT *F1, INPUT *F2)
{
	u_long cnt1, cnt2;

	/*
	 * Output the results of a join comparison.  The output may be from
	 * either file 1 or file 2 (in which case the first argument is the
	 * file from which to output) or from both.
	 */
	if (F2 == NULL) {
		for (cnt1 = 0; cnt1 < F1->setcnt; ++cnt1)
			outoneline(F1, &F1->set[cnt1]);
		return;
	}
	for (cnt1 = 0; cnt1 < F1->setcnt; ++cnt1)
		for (cnt2 = 0; cnt2 < F2->setcnt; ++cnt2)
			outtwoline(F1, &F1->set[cnt1], F2, &F2->set[cnt2]);
}

void
outoneline(INPUT *F, LINE *lp)
{
	u_long cnt;

	/*
	 * Output a single line from one of the files, according to the
	 * join rules.  This happens when we are writing unmatched single
	 * lines.  Output empty fields in the right places.
	 */
	if (olist)
		for (cnt = 0; cnt < olistcnt; ++cnt) {
			if (olist[cnt].filenum == F->number)
				outfield(lp, olist[cnt].fieldno, 0);
			else if (olist[cnt].filenum == 0)
				outfield(lp, F->joinf, 0);
			else
				outfield(lp, 0, 1);
		}
	else {
		/*
		 * Output the join field, then the remaining fields from F
		 */
		outfield(lp, F->joinf, 0);
		for (cnt = 0; cnt < lp->fieldcnt; ++cnt)
			if (F->joinf != cnt)
				outfield(lp, cnt, 0);
	}

	putchar('\n');
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */
	needsep = 0;
}

void
outtwoline(INPUT *F1, LINE *lp1, INPUT *F2, LINE *lp2)
{
	u_long cnt;

	/* Output a pair of lines according to the join list (if any). */
	if (olist) {
		for (cnt = 0; cnt < olistcnt; ++cnt)
			if (olist[cnt].filenum == 0) {
				if (lp1->fieldcnt >= F1->joinf)
					outfield(lp1, F1->joinf, 0);
				else
					outfield(lp2, F2->joinf, 0);
			} else if (olist[cnt].filenum == 1)
				outfield(lp1, olist[cnt].fieldno, 0);
			else /* if (olist[cnt].filenum == 2) */
				outfield(lp2, olist[cnt].fieldno, 0);
	} else {
		/*
		 * Output the join field, then the remaining fields from F1
		 * and F2.
		 */
		outfield(lp1, F1->joinf, 0);
		for (cnt = 0; cnt < lp1->fieldcnt; ++cnt)
			if (F1->joinf != cnt)
				outfield(lp1, cnt, 0);
		for (cnt = 0; cnt < lp2->fieldcnt; ++cnt)
			if (F2->joinf != cnt)
				outfield(lp2, cnt, 0);
	}
	putchar('\n');
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */
	needsep = 0;
}

void
outfield(LINE *lp, u_long fieldno, int out_empty)
{
	if (needsep++)
		/*
		 * amiport: replaced putwchar(*tabchar) with putchar((unsigned char)tabchar[0])
		 * -- putwchar() is a wchar function (wchar.h removed). tabchar is now char[],
		 * tabchar[0] holds the active delimiter character.
		 */
		putchar((unsigned char)tabchar[0]); /* amiport: putwchar -> putchar, char delimiter */
	if (!ferror(stdout)) {
		if (lp->fieldcnt <= fieldno || out_empty) {
			if (empty != NULL)
				fputs(empty, stdout);
		} else {
			if (*lp->fields[fieldno] == '\0')
				return;
			fputs(lp->fields[fieldno], stdout);
		}
	}
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */
}

/*
 * Convert an output list argument "2.1, 1.3, 2.4" into an array of output
 * fields.
 */
void
fieldarg(char *option)
{
	u_long fieldno = 0, filenum = 0; /* amiport: init to silence GCC -- errx() exits before uninitialized use */
	char *end, *token;

	while ((token = strsep(&option, ", \t")) != NULL) {
		if (*token == '\0')
			continue;
		if (token[0] == '0')
			filenum = fieldno = 0;
		else if ((token[0] == '1' || token[0] == '2') &&
		    token[1] == '.') {
			filenum = token[0] - '0';
			fieldno = strtol(token + 2, &end, 10);
			if (*end)
				errx(10, "malformed -o option field"); /* amiport: RETURN_ERROR */
			if (fieldno == 0)
				errx(10, "field numbers are 1 based"); /* amiport: RETURN_ERROR */
			--fieldno;
		} else
			errx(10, "malformed -o option field"); /* amiport: RETURN_ERROR */
		if (olistcnt == olistalloc) {
			OLIST *p;
			u_long newsize = olistalloc + 50;
			/* amiport: reallocarray() provided by <amiport/string.h> macro */
			if ((p = reallocarray(olist, newsize, sizeof(OLIST)))
			    == NULL)
				err(10, NULL); /* amiport: RETURN_ERROR */
			olist = p;
			olistalloc = newsize;
		}
		olist[olistcnt].filenum = filenum;
		olist[olistcnt].fieldno = fieldno;
		++olistcnt;
	}
}

void
obsolete(char **argv)
{
	size_t len;
	char **p, *ap, *t;

	while ((ap = *++argv) != NULL) {
		/* Return if "--". */
		if (ap[0] == '-' && ap[1] == '-')
			return;
		/* skip if not an option */
		if (ap[0] != '-')
			continue;
		switch (ap[1]) {
		case 'a':
			/*
			 * The original join allowed "-a", which meant the
			 * same as -a1 plus -a2.  POSIX 1003.2, Draft 11.2
			 * only specifies this as "-a 1" and "a -2", so we
			 * have to use another option flag, one that is
			 * unlikely to ever be used or accidentally entered
			 * on the command line.  (Well, we could reallocate
			 * the argv array, but that hardly seems worthwhile.)
			 */
			if (ap[2] == '\0' && (argv[1] == NULL ||
			    (strcmp(argv[1], "1") != 0 &&
			    strcmp(argv[1], "2") != 0))) {
				ap[1] = '\01';
				warnx("-a option used without an argument; "
				    "reverting to historical behavior");
			}
			break;
		case 'j':
			/*
			 * The original join allowed "-j[12] arg" and "-j arg".
			 * Convert the former to "-[12] arg".  Don't convert
			 * the latter since getopt(3) can handle it.
			 */
			switch(ap[2]) {
			case '1':
			case '2':
				if (ap[3] != '\0')
					goto jbad;
				ap[1] = ap[2];
				ap[2] = '\0';
				break;
			case '\0':
				break;
			default:
jbad:				warnx("unknown option -- %s", ap + 1);
				usage();
			}
			break;
		case 'o':
			/*
			 * The original join allowed "-o arg arg".
			 * Convert to "-o arg -o arg".
			 */
			if (ap[2] != '\0' || argv[1] == NULL)
				break;
			for (p = argv + 2; *p != NULL; ++p) {
				if (p[0][0] == '0' || ((p[0][0] != '1' &&
				    p[0][0] != '2') || p[0][1] != '.'))
					break;
				len = strlen(*p);
				if (len - 2 != strspn(*p + 2, "0123456789"))
					break;
				if ((t = malloc(len + 3)) == NULL)
					err(10, NULL); /* amiport: RETURN_ERROR */
				/* amiport: track for atexit cleanup -- AmigaOS has no GC */
				if (obsolete_alloc_count < MAX_OBSOLETE_ALLOCS)
					obsolete_allocs[obsolete_alloc_count++] = t;
				t[0] = '-';
				t[1] = 'o';
				memmove(t + 2, *p, len + 1);
				*p = t;
			}
			argv = p - 1;
			break;
		}
	}
}

void
usage(void)
{
	int len;
	extern char *__progname;

	len = strlen(__progname) + sizeof("usage: ");
	(void)fprintf(stderr, "usage: %s [-1 field] [-2 field] "
	    "[-a file_number | -v file_number] [-e string]\n"
	    "%*s[-o list] [-t char] file1 file2\n",
	    __progname, len, "");
	exit(10); /* amiport: RETURN_ERROR -- was exit(1) */
}
