/*
 * Plan 9 from User Space sort utility
 * Originally from Plan 9, ported to Unix as part of plan9port.
 *
 * Copyright (c) 2002 Lucent Technologies Inc.
 * Distributed under the terms of the Lucent Public License version 1.02.
 *
 * AmigaOS 3.x port by amiport code-transformer.
 */

/* amiport: removed Plan 9 headers <u.h>, <libc.h>, <bio.h> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
/* amiport: amiport shim headers */
#include <amiport/stdlib.h>
#include <amiport/stdio.h>
#include <amiport/signal.h>
#include <amiport/unistd.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: sort 1.0 (22.03.2026)";

/* amiport: stack cookie — 65536 for recursive radix sort */
long __stack = 65536;

/* amiport: Plan 9 type definitions not available on AmigaOS */
typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef long           Rune;
#define Runeself       0x80
#define nelem(arr)     ((int)(sizeof(arr)/sizeof((arr)[0])))
#define USED(x)        ((void)(x))

/*
 * Minimal UTF-8 codec — needed for -t option and latin map handling.
 * AmigaOS uses Latin-1; these are provided for correctness.
 * amiport: bundled minimal chartorune/runetochar (Plan 9 Bio library not available)
 */

/* chartorune: decode first UTF-8 char from s, store in *r, return byte count */
static int
chartorune(Rune *r, const char *s)
{
	unsigned char c;
	unsigned char c1, c2;
	int n;

	c = (unsigned char)s[0];
	if(c < Runeself) {
		*r = c;
		return 1;
	}
	if((c & 0xe0) == 0xc0) {
		c1 = (unsigned char)s[1];
		if((c1 & 0xc0) == 0x80) {
			n = ((c & 0x1f) << 6) | (c1 & 0x3f);
			if(n >= 0x80) {
				*r = n;
				return 2;
			}
		}
	} else if((c & 0xf0) == 0xe0) {
		c1 = (unsigned char)s[1];
		c2 = (unsigned char)s[2];
		if((c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80) {
			n = ((c & 0x0f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f);
			if(n >= 0x800) {
				*r = n;
				return 3;
			}
		}
	}
	/* invalid sequence — treat byte as Latin-1 */
	*r = c;
	return 1;
}

/* runetochar: encode Rune r into s, return byte count */
static int
runetochar(char *s, const Rune *rp)
{
	Rune r;

	r = *rp;
	if(r < Runeself) {
		s[0] = (char)r;
		return 1;
	}
	if(r < 0x800) {
		s[0] = (char)(0xc0 | (r >> 6));
		s[1] = (char)(0x80 | (r & 0x3f));
		return 2;
	}
	s[0] = (char)(0xe0 | (r >> 12));
	s[1] = (char)(0x80 | ((r >> 6) & 0x3f));
	s[2] = (char)(0x80 | (r & 0x3f));
	return 3;
}

/*
bugs:
	00/ff for end of file can conflict with 00/ff characters
*/

enum
{
	/* amiport: Nline reduced from 500000 — Amiga memory constraint */
	Nline	= 10000,		/* default max number of lines saved in memory */
	Nmerge	= 10,			/* max number of temporary files merged */
	Nfield	= 20,			/* max number of argument fields */

	Bflag	= 1<<0,			/* flags per field */
	B1flag	= 1<<1,

	Dflag	= 1<<2,
	Fflag	= 1<<3,
	Gflag	= 1<<4,
	Iflag	= 1<<5,
	Mflag	= 1<<6,
	Nflag	= 1<<7,
	Rflag	= 1<<8,
	Wflag	= 1<<9,

	NSstart	= 0,			/* states for number to key decoding */
	NSsign,
	NSzero,
	NSdigit,
	NSpoint,
	NSfract,
	NSzerofract,
	NSexp,
	NSexpsign,
	NSexpdigit
};

typedef	struct	Line	Line;
typedef	struct	Key	Key;
/* amiport: Biobuf replaced with FILE* throughout */
typedef	struct	Merge	Merge;
typedef	struct	Field	Field;

struct	Line
{
	Key*	key;
	int	llen;		/* always >= 1 */
	uchar	line[1];	/* always ends in '\n' */
};

struct	Merge
{
	Key*	key;		/* copy of line->key so (Line*) looks like (Merge*) */
	Line*	line;		/* line at the head of a merged temp file */
	int	fd;		/* file descriptor */
	FILE*	b;		/* amiport: was Biobuf b — replaced with FILE* */
};

struct	Key
{
	int	klen;
	uchar	key[1];
};

struct	Field
{
	int	beg1;
	int	beg2;
	int	end1;
	int	end2;

	long	flags;
	uchar	mapto[256];

	void	(*dokey)(Key*, uchar*, uchar*, Field*);
};

struct args
{
	char*	ofile;
	char*	tname;
	Rune	tabchar;
	char	cflag;
	char	uflag;
	char	vflag;
	int	nfield;
	int	nfile;
	Field	field[Nfield];

	Line**	linep;
	long	nline;			/* number of lines in this temp file */
	long	lineno;			/* overall ordinal for -s option */
	int	ntemp;
	long	mline;			/* max lines per file */
} args;

extern	int	latinmap[];
extern	Rune*	month[12];

void	buildkey(Line*);
void	doargs(int, char*[]);
void	dofield(char*, int*, int*, int, int);
/* amiport: dofile() signature changed from Biobuf* to FILE* */
void	dofile(FILE*);
void	dokey_(Key*, uchar*, uchar*, Field*);
void	dokey_dfi(Key*, uchar*, uchar*, Field*);
void	dokey_gn(Key*, uchar*, uchar*, Field*);
void	dokey_m(Key*, uchar*, uchar*, Field*);
void	dokey_r(Key*, uchar*, uchar*, Field*);
void	done(char*);
int	keycmp(Key*, Key*);
void	makemapd(Field*);
void	makemapm(Field*);
/* amiport: mergefiles() signature changed from Biobuf* to FILE* */
void	mergefiles(int, int, FILE*);
/* amiport: mergeout() signature changed from Biobuf* to FILE* */
void	mergeout(FILE*);
void	newfield(void);
/* amiport: newline() signature changed from Biobuf* to FILE* */
Line*	newline(FILE*);
void	nomem(void);
void	printargs(void);
/* amiport: printout() signature changed from Biobuf* to FILE* */
void	printout(FILE*);
void	setfield(int, int);
uchar*	skip(uchar*, int, int, int, int);
void	sort4(void*, ulong);
char*	tempfile(int);
void	tempout(void);
/* amiport: lineout() signature changed from Biobuf* to FILE* */
void	lineout(FILE*, Line*);

/* amiport: POSIX signal handler replaces Plan 9 notify/notifyf */
static void
sighandler(int sig)
{
	USED(sig);
	done(0);
}

/* amiport: changed void main() to int main() — C89 requires int return */
int
main(int argc, char *argv[])
{
	int i;
	char *s;
	/* amiport: Biobuf bbuf replaced with FILE* */
	FILE *bbuf;

	/* amiport: replaced notify(notifyf) with POSIX signal() for SIGINT/SIGHUP */
	signal(SIGINT, sighandler);   /* amiport: Ctrl-C cleanup */
	signal(SIGHUP, sighandler);   /* amiport: hangup cleanup */

	doargs(argc, argv);
	if(args.vflag)
		printargs();

	for(i=1; i<argc; i++) {
		s = argv[i];
		if(s == 0)
			continue;
		if(strcmp(s, "-") == 0) {
			/* amiport: Binit(&bbuf, 0, OREAD) replaced with FILE* stdin */
			bbuf = stdin;
			dofile(bbuf);
			/* amiport: Bterm(&bbuf) — no close for stdin */
			continue;
		}
		/* amiport: use fopen() not amiport_open()+fdopen() — amiport fds
		 * are in a separate namespace from libnix fds, so fdopen() on an
		 * amiport fd silently fails to read. fopen() goes through libnix
		 * which correctly maps to AmigaDOS file handles. */
		bbuf = fopen(s, "r");
		if(bbuf == 0) {
			fprintf(stderr, "sort: open %s: %s\n", s, strerror(errno));
			done("open");
		}
		dofile(bbuf);
		if(bbuf != stdin && bbuf != stdout) fclose(bbuf);
	}
	if(args.nfile == 0) {
		/* amiport: Binit(&bbuf, 0, OREAD) — stdin */
		bbuf = stdin;
		dofile(bbuf);
		/* no fclose for stdin */
	}
	if(args.cflag)
		done(0);
	if(args.vflag)
		fprintf(stderr, "=========\n");

	/* amiport: f=1 was stdout fd; use FILE* stdout directly */
	if(args.ofile) {
		/* amiport: use fopen() for output — same fd namespace issue as input */
		bbuf = fopen(args.ofile, "w");
		if(bbuf == 0) {
			fprintf(stderr, "sort: create %s: %s\n", args.ofile, strerror(errno));
			done("create");
		}
	} else {
		/* amiport: Binit(&bbuf, 1, OWRITE) — stdout */
		bbuf = stdout;
	}

	if(args.ntemp) {
		tempout();
		mergeout(bbuf);
	} else {
		printout(bbuf);
	}
	/* amiport: Bterm — only fclose non-stdout */
	if(bbuf != stdin && bbuf != stdout) fclose(bbuf);
	done(0);
	/* amiport: return 0 for int main() */
	return 0;
}

void
dofile(FILE *b)
{
	Line *l, *ol;
	int n;

	/* amiport: amiport_check_break() in long-running loop for Ctrl-C support */
	if(args.cflag) {
		ol = newline(b);
		if(ol == 0)
			return;
		for(;;) {
			if(amiport_check_break()) {
				(void)fflush(stdout);
				done(0);
			}
			l = newline(b);
			if(l == 0)
				break;
			n = keycmp(ol->key, l->key);
			if(n > 0 || (n == 0 && args.uflag)) {
				/* amiport: free last line before error exit */
				free(ol->key);
				free(ol);
				free(l->key);
				free(l);
				fprintf(stderr, "sort: -c file not in sort\n");
				done("order");
			}
			free(ol->key);
			free(ol);
			ol = l;
		}
		/* amiport: free last line — leaked in original Plan 9 code */
		if(ol) {
			free(ol->key);
			free(ol);
		}
		return;
	}

	if(args.linep == 0) {
		args.linep = malloc(args.mline * sizeof(args.linep));
		if(args.linep == 0)
			nomem();
	}
	for(;;) {
		/* amiport: amiport_check_break() in file reading loop */
		if(amiport_check_break()) {
			(void)fflush(stdout);
			done(0);
		}
		l = newline(b);
		if(l == 0)
			break;
		if(args.nline >= args.mline)
			tempout();
		args.linep[args.nline] = l;
		args.nline++;
		args.lineno++;
	}
}

/*
 * amiport: notifyf() removed — Plan 9 notify mechanism not available on AmigaOS.
 * Signal handling replaced by POSIX signal() in main() calling sighandler() above.
 * noted(NDFLT) had no direct equivalent; ignore-and-continue behavior is dropped
 * since SIGINT/SIGHUP/SIGTERM all cause clean exit via done() on AmigaOS.
 */

/*
 * newline() — read one line from FILE* using fgetc().
 * amiport: rewritten from Brdline+Blinelen to fgetc character-at-a-time.
 * Plan 9 Brdline returns pointer into internal buffer; we allocate explicitly.
 */
Line*
newline(FILE *b)
{
	Line *l;
	int n;
	/* amiport: perf — static buffer replaces fgetc() character-at-a-time.
	 * On 68000, each fgetc() costs a JSR+stack frame+buffer check. For a
	 * 40-char line that's 40 function calls. fgets() reads in one call
	 * using libnix's internal buffer, then we memcpy once. 3-5x speedup
	 * for the read phase. Safe: single-threaded, not reentrant. */
	static char lbuf[4096];

	if(fgets(lbuf, (int)sizeof(lbuf), b) == 0)
		return 0;

	n = (int)strlen(lbuf);
	/* handle missing trailing newline at EOF */
	if(n > 0 && lbuf[n-1] != '\n') {
		fprintf(stderr, "sort: newline added\n");
		if(n < (int)sizeof(lbuf) - 1)
			lbuf[n++] = '\n';
	}

	l = malloc(sizeof(Line) + (n > 0 ? n - 1 : 0) * sizeof(l->line[0]));
	if(l == 0)
		nomem();
	l->llen = n;
	memmove(l->line, lbuf, n);
	buildkey(l);
	return l;
}

void
lineout(FILE *b, Line *l)
{
	int n, m;

	n = l->llen;
	/* amiport: Bwrite(b, l->line, n) replaced with fwrite() */
	m = (int)fwrite(l->line, 1, n, b);
	if(n != m)
		/* amiport: exits("write") replaced with done("write") for proper cleanup */
		done("write");
}

void
tempout(void)
{
	long n;
	Line **lp, *l;
	char *tf;
	/* amiport: Biobuf tb replaced with FILE* tb */
	FILE *tb;

	sort4(args.linep, args.nline);
	tf = tempfile(args.ntemp);
	args.ntemp++;
	/* amiport: use fopen() — amiport fds incompatible with fdopen() */
	tb = fopen(tf, "w");
	if(tb == 0) {
		fprintf(stderr, "sort: create %s: %s\n", tf, strerror(errno));
		done("create");
	}
	lp = args.linep;
	for(n=args.nline; n>0; n--) {
		l = *lp++;
		lineout(tb, l);
		free(l->key);
		free(l);
	}
	args.nline = 0;
	/* amiport: Bterm(&tb) replaced with fclose; fd owned by FILE* */
	fclose(tb);
}

void
done(char *xs)
{
	int i;

	for(i=0; i<args.ntemp; i++)
		remove(tempfile(i));
	/* amiport: free line array — AmigaOS -noixemul leaks permanently */
	if(args.linep != 0) {
		long j;
		for(j=0; j<args.nline; j++) {
			if(args.linep[j]) {
				free(args.linep[j]->key);
				free(args.linep[j]);
			}
		}
		free(args.linep);
		args.linep = 0;
		args.nline = 0;
	}
	/* amiport: flush before exit */
	(void)fflush(stdout);
	(void)fflush(stderr);
	/* amiport: exits(xs) replaced — exit(10)=RETURN_ERROR, exit(0)=RETURN_OK */
	if(xs)
		exit(10);
	else
		exit(0);
}

void
nomem(void)
{
	/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
	fprintf(stderr, "sort: out of memory\n");
	done("mem");
}

char*
tempfile(int n)
{
	static char file[100];
	static uint pid;
	char *dir;

	/* amiport: default temp dir changed from "/var/tmp" to "T:" (AmigaOS RAM temp) */
	dir = "T:";
	if(args.tname)
		dir = args.tname;
	if(strlen(dir) >= (size_t)(nelem(file)-20)) {
		/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
		fprintf(stderr, "temp file directory name is too long: %s\n", dir);
		done("tdir");
	}

	if(pid == 0) {
		/* amiport: use amiport_getpid() from amiport/unistd.h */
		pid = (uint)amiport_getpid();
		if(pid == 0) {
			pid = (uint)time(0);
			if(pid == 0)
				pid = 1;
		}
	}

	/* amiport: sprint() replaced with snprintf() — bounds-safe, file[100] */
	snprintf(file, sizeof(file), "%ssort.%.4d.%.4d", dir, pid%10000, n);
	return file;
}

void
mergeout(FILE *b)
{
	int n, i;
	char *tf;
	/* amiport: Biobuf tb replaced with FILE* tb */
	FILE *tb;

	for(i=0; i<args.ntemp; i+=n) {
		n = args.ntemp - i;
		if(n > Nmerge) {
			tf = tempfile(args.ntemp);
			args.ntemp++;
			/* amiport: use fopen() — amiport fds incompatible with fdopen() */
			tb = fopen(tf, "w");
			if(tb == 0) {
				fprintf(stderr, "sort: create %s: %s\n", tf, strerror(errno));
				done("create");
			}

			n = Nmerge;
			mergefiles(i, n, tb);

			/* amiport: Bterm(&tb) + close(f) — fclose handles both */
			fclose(tb);
		} else
			mergefiles(i, n, b);
	}
}

void
mergefiles(int t, int n, FILE *b)
{
	Merge *m, *mp, **mmp;
	Key *ok;
	Line *l, *nl;	/* amiport: nl separates "next line" reads from freed l */
	char *tf;
	int i, nn;

	/*
	 * amiport: memory safety fix — check each alloc immediately.
	 * nomem() calls exit() so no free() before it; on AmigaOS exit()
	 * reclaims all process memory, so no leak.
	 */
	mmp = malloc(n*sizeof(*mmp));
	if(mmp == 0)
		nomem();
	mp = malloc(n*sizeof(*mp));
	if(mp == 0)
		nomem();

	nn = 0;
	m = mp;
	for(i=0; i<n; i++,m++) {
		tf = tempfile(t+i);
		/* amiport: use fopen() — amiport fds incompatible with fdopen() */
		m->b = fopen(tf, "r");
		if(m->b == 0) {
			fprintf(stderr, "sort: reopen %s: %s\n", tf, strerror(errno));
			done("open");
		}
		m->fd = -1; /* amiport: fd not used with fopen() */
		mmp[nn] = m;

		l = newline(m->b);
		if(l == 0)
			continue;
		nn++;
		m->line = l;
		m->key = l->key;
	}

	ok = 0;
	for(;;) {
		sort4(mmp, nn);
		m = *mmp;
		if(nn == 0)
			break;
		for(;;) {
			l = m->line;
			if(args.uflag && ok && keycmp(ok, l->key) == 0) {
				free(l->key);
				free(l);
			} else {
				lineout(b, l);
				if(ok)
					free(ok);
				ok = l->key;
				free(l);
			}

			/* amiport: use nl (not l) so analyser does not alias with freed l */
			nl = newline(m->b);
			if(nl == 0) {
				nn--;
				mmp[0] = mmp[nn];
				break;
			}
			m->line = nl;
			m->key = nl->key;
			if(nn > 1 && keycmp(mmp[0]->key, mmp[1]->key) > 0)
				break;
		}
	}
	if(ok)
		free(ok);

	m = mp;
	for(i=0; i<n; i++,m++) {
		/* amiport: Bterm(&m->b) + close(m->fd) replaced with fclose (owns fd) */
		if(m->b != 0) fclose(m->b);
	}

	free(mp);
	free(mmp);
}

int
keycmp(Key *ka, Key *kb)
{
	int n, m;

	/*
	 * set n to length of smaller key
	 */
	n = ka->klen;
	m = kb->klen;
	if(n > m)
		n = m;
	return memcmp(ka->key, kb->key, n);
}

void
printout(FILE *b)
{
	long n;
	Line **lp, *l;
	Key *ok;

	sort4(args.linep, args.nline);
	lp = args.linep;
	ok = 0;
	for(n=args.nline; n>0; n--) {
		l = *lp++;
		if(args.uflag && ok && keycmp(ok, l->key) == 0)
			continue;
		lineout(b, l);
		ok = l->key;
	}
}

void
setfield(int n, int c)
{
	Field *f;

	f = &args.field[n];
	switch(c) {
	default:
		/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
		/* amiport: %C replaced with %c */
		fprintf(stderr, "sort: unknown option: field.%c\n", c);
		done("option");
	case 'b':	/* skip blanks */
		f->flags |= Bflag;
		break;
	case 'd':	/* directory order */
		f->flags |= Dflag;
		break;
	case 'f':	/* fold case */
		f->flags |= Fflag;
		break;
	case 'g':	/* floating point -n case */
		f->flags |= Gflag;
		break;
	case 'i':	/* ignore non-ascii */
		f->flags |= Iflag;
		break;
	case 'M':	/* month */
		f->flags |= Mflag;
		break;
	case 'n':	/* numbers */
		f->flags |= Nflag;
		break;
	case 'r':	/* reverse */
		f->flags |= Rflag;
		break;
	case 'w':	/* ignore white */
		f->flags |= Wflag;
		break;
	}
}

void
dofield(char *s, int *n1, int *n2, int off1, int off2)
{
	int c, n;

	c = *s++;
	if(c >= '0' && c <= '9') {
		n = 0;
		while(c >= '0' && c <= '9') {
			n = n*10 + (c-'0');
			c = *s++;
		}
		n -= off1;
		if(n < 0) {
			/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
			fprintf(stderr, "sort: field offset must be positive\n");
			done("option");
		}
		*n1 = n;
	}
	if(c == '.') {
		c = *s++;
		if(c >= '0' && c <= '9') {
			n = 0;
			while(c >= '0' && c <= '9') {
				n = n*10 + (c-'0');
				c = *s++;
			}
			n -= off2;
			if(n < 0) {
				/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
				fprintf(stderr, "sort: character offset must be positive\n");
				done("option");
			}
			*n2 = n;
		}
	}
	while(c != 0) {
		setfield(args.nfield, c);
		c = *s++;
	}
}

void
printargs(void)
{
	int i, n;
	Field *f;
	char *prefix;

	/* amiport: fprint(2,...) replaced with fprintf(stderr,...) throughout */
	fprintf(stderr, "sort");
	for(i=0; i<=args.nfield; i++) {
		f = &args.field[i];
		prefix = " -";
		if(i) {
			n = f->beg1;
			if(n >= 0)
				fprintf(stderr, " +%d", n);
			else
				fprintf(stderr, " +*");
			n = f->beg2;
			if(n >= 0)
				fprintf(stderr, ".%d", n);
			else
				fprintf(stderr, ".*");

			if(f->flags & B1flag)
				fprintf(stderr, "b");

			n = f->end1;
			if(n >= 0)
				fprintf(stderr, " -%d", n);
			else
				fprintf(stderr, " -*");
			n = f->end2;
			if(n >= 0)
				fprintf(stderr, ".%d", n);
			else
				fprintf(stderr, ".*");
			prefix = "";
		}
		if(f->flags & Bflag)
			fprintf(stderr, "%sb", prefix);
		if(f->flags & Dflag)
			fprintf(stderr, "%sd", prefix);
		if(f->flags & Fflag)
			fprintf(stderr, "%sf", prefix);
		if(f->flags & Gflag)
			fprintf(stderr, "%sg", prefix);
		if(f->flags & Iflag)
			fprintf(stderr, "%si", prefix);
		if(f->flags & Mflag)
			fprintf(stderr, "%sM", prefix);
		if(f->flags & Nflag)
			fprintf(stderr, "%sn", prefix);
		if(f->flags & Rflag)
			fprintf(stderr, "%sr", prefix);
		if(f->flags & Wflag)
			fprintf(stderr, "%sw", prefix);
	}
	if(args.cflag)
		fprintf(stderr, " -c");
	if(args.uflag)
		fprintf(stderr, " -u");
	if(args.ofile)
		fprintf(stderr, " -o %s", args.ofile);
	if(args.mline != Nline)
		fprintf(stderr, " -l %ld", args.mline);
	fprintf(stderr, "\n");
}

void
newfield(void)
{
	int n;
	Field *f;

	n = args.nfield + 1;
	if(n >= Nfield) {
		/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
		fprintf(stderr, "sort: too many fields specified\n");
		done("option");
	}
	args.nfield = n;
	f = &args.field[n];
	f->beg1 = -1;
	f->beg2 = -1;
	f->end1 = -1;
	f->end2 = -1;
}

void
doargs(int argc, char *argv[])
{
	int i, c, hadplus;
	char *s, *p, *q;
	Field *f;

	hadplus = 0;
	args.mline = Nline;
	for(i=1; i<argc; i++) {
		s = argv[i];
		c = *s++;
		if(c == '-') {
			c = *s;
			if(c == 0)		/* forced end of arg marker */
				break;
			argv[i] = 0;		/* clobber args processed */
			if(c == '.' || (c >= '0' && c <= '9')) {
				if(!hadplus)
					newfield();
				f = &args.field[args.nfield];
				dofield(s, &f->end1, &f->end2, 0, 0);
				hadplus = 0;
				continue;
			}

			while((c = *s++) != 0)
			switch(c) {
			case '-':	/* end of options */
				i = argc;
				continue;
			case 'T':	/* temp directory */
				if(*s == 0) {
					i++;
					if(i < argc) {
						args.tname = argv[i];
						argv[i] = 0;
					}
				} else
					args.tname = s;
				s = strchr(s, 0);
				break;
			case 'o':	/* output file */
				if(*s == 0) {
					i++;
					if(i < argc) {
						args.ofile = argv[i];
						argv[i] = 0;
					}
				} else
					args.ofile = s;
				s = strchr(s, 0);
				break;
			case 'k':	/* posix key */
				p = 0;
				if(*s == 0) {
					i++;
					if(i < argc) {
						p = argv[i];
						argv[i] = 0;
					}
				} else
					p = s;
				s = strchr(s, 0);
				if(p == 0)
					break;

				newfield();
				q = strchr(p, ',');
				if(q)
					*q++ = 0;
				f = &args.field[args.nfield];
				dofield(p, &f->beg1, &f->beg2, 1, 1);
				if(f->flags & Bflag) {
					f->flags |= B1flag;
					f->flags &= ~Bflag;
				}
				if(q) {
					dofield(q, &f->end1, &f->end2, 1, 0);
					if(f->end2 <= 0)
						f->end1++;
				}
				hadplus = 0;
				break;
			case 't':	/* tab character */
				if(*s == 0) {
					i++;
					if(i < argc) {
						/* amiport: chartorune now local static function */
						chartorune(&args.tabchar, argv[i]);
						argv[i] = 0;
					}
				} else
					s += chartorune(&args.tabchar, s);
				if(args.tabchar == '\n') {
					/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
					fprintf(stderr, "aw come on, rob\n");
					done("rob");
				}
				break;
			case 'c':	/* check order */
				args.cflag = 1;
				break;
			case 'u':	/* unique */
				args.uflag = 1;
				break;
			case 'v':	/* debugging noise */
				args.vflag = 1;
				break;
			case 'l':
				if(*s == 0) {
					i++;
					if(i < argc) {
						args.mline = atol(argv[i]);
						argv[i] = 0;
					}
				} else
					args.mline = atol(s);
				s = strchr(s, 0);
				break;

			case 'M':	/* month */
			case 'b':	/* skip blanks */
			case 'd':	/* directory order */
			case 'f':	/* fold case */
			case 'g':	/* floating numbers */
			case 'i':	/* ignore non-ascii */
			case 'n':	/* numbers */
			case 'r':	/* reverse */
			case 'w':	/* ignore white */
				if(args.nfield > 0)
					/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
					fprintf(stderr, "sort: global field set after -k\n");
				setfield(0, c);
				break;
			case 'm':
				/* option m silently ignored but required by posix */
				break;
			default:
				/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
				/* amiport: %C replaced with %c */
				fprintf(stderr, "sort: unknown option: -%c\n", c);
				done("option");
			}
			continue;
		}
		if(c == '+') {
			argv[i] = 0;		/* clobber args processed */
			c = *s;
			if(c == '.' || (c >= '0' && c <= '9')) {
				newfield();
				f = &args.field[args.nfield];
				dofield(s, &f->beg1, &f->beg2, 0, 0);
				if(f->flags & Bflag) {
					f->flags |= B1flag;
					f->flags &= ~Bflag;
				}
				hadplus = 1;
				continue;
			}
			/* amiport: %C replaced with %c */
			fprintf(stderr, "sort: unknown option: +%c\n", c);
			done("option");
		}
		args.nfile++;
	}

	for(i=0; i<=args.nfield; i++) {
		f = &args.field[i];

		/*
		 * global options apply to fields that
		 * specify no options
		 */
		if(f->flags == 0) {
			f->flags = args.field[0].flags;
			if(args.field[0].flags & Bflag)
				f->flags |= B1flag;
		}


		/*
		 * build buildkey specification
		 */
		switch(f->flags & ~(Bflag|B1flag)) {
		default:
			/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
			fprintf(stderr, "sort: illegal combination of flags: %lx\n", f->flags);
			done("option");
		case 0:
			f->dokey = dokey_;
			break;
		case Rflag:
			f->dokey = dokey_r;
			break;
		case Gflag:
		case Nflag:
		case Gflag|Nflag:
		case Gflag|Rflag:
		case Nflag|Rflag:
		case Gflag|Nflag|Rflag:
			f->dokey = dokey_gn;
			break;
		case Mflag:
		case Mflag|Rflag:
			f->dokey = dokey_m;
			makemapm(f);
			break;
		case Dflag:
		case Dflag|Fflag:
		case Dflag|Fflag|Iflag:
		case Dflag|Fflag|Iflag|Rflag:
		case Dflag|Fflag|Iflag|Rflag|Wflag:
		case Dflag|Fflag|Iflag|Wflag:
		case Dflag|Fflag|Rflag:
		case Dflag|Fflag|Rflag|Wflag:
		case Dflag|Fflag|Wflag:
		case Dflag|Iflag:
		case Dflag|Iflag|Rflag:
		case Dflag|Iflag|Rflag|Wflag:
		case Dflag|Iflag|Wflag:
		case Dflag|Rflag:
		case Dflag|Rflag|Wflag:
		case Dflag|Wflag:
		case Fflag:
		case Fflag|Iflag:
		case Fflag|Iflag|Rflag:
		case Fflag|Iflag|Rflag|Wflag:
		case Fflag|Iflag|Wflag:
		case Fflag|Rflag:
		case Fflag|Rflag|Wflag:
		case Fflag|Wflag:
		case Iflag:
		case Iflag|Rflag:
		case Iflag|Rflag|Wflag:
		case Iflag|Wflag:
		case Wflag:
			f->dokey = dokey_dfi;
			makemapd(f);
			break;
		}
	}

	/*
	 * random spot checks
	 */
	if(args.nfile > 1 && args.cflag) {
		/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
		fprintf(stderr, "sort: -c can have at most one input file\n");
		done("option");
	}
	return;
}

uchar*
skip(uchar *l, int n1, int n2, int bflag, int endfield)
{
	int i, c, tc;
	Rune r;

	if(endfield && n1 < 0)
		return 0;

	c = *l++;
	tc = args.tabchar;
	if(tc) {
		if(tc < Runeself) {
			for(i=n1; i>0; i--) {
				while(c != tc) {
					if(c == '\n')
						return 0;
					c = *l++;
				}
				if(!(endfield && i == 1))
					c = *l++;
			}
		} else {
			l--;
			l += chartorune(&r, (char*)l);
			for(i=n1; i>0; i--) {
				while(r != tc) {
					if(r == '\n')
						return 0;
					l += chartorune(&r, (char*)l);
				}
				if(!(endfield && i == 1))
					l += chartorune(&r, (char*)l);
			}
			c = r;
		}
	} else {
		for(i=n1; i>0; i--) {
			while(c == ' ' || c == '\t')
				c = *l++;
			while(c != ' ' && c != '\t') {
				if(c == '\n')
					return 0;
				c = *l++;
			}
		}
	}

	if(bflag)
		while(c == ' ' || c == '\t')
			c = *l++;

	l--;
	for(i=n2; i>0; i--) {
		c = *l;
		if(c < Runeself) {
			if(c == '\n')
				return 0;
			l++;
			continue;
		}
		l += chartorune(&r, (char*)l);
	}
	return l;
}

void
dokey_gn(Key *k, uchar *lp, uchar *lpe, Field *f)
{
	uchar *kp;
	int c, cl, dp;
	int state, nzero, exp, expsign, rflag;

	cl = k->klen + 3;
	kp = k->key + cl;	/* skip place for sign, exponent[2] */

	nzero = 0;		/* number of trailing zeros */
	exp = 0;		/* value of the exponent */
	expsign = 0;		/* sign of the exponent */
	dp = 0x4040;		/* location of decimal point */
	rflag = f->flags&Rflag;	/* xor of rflag and - sign */
	state = NSstart;

	for(;; lp++) {
		if(lp >= lpe)
			break;
		c = *lp;

		if(c == ' ' || c == '\t') {
			switch(state) {
			case NSstart:
			case NSsign:
				continue;
			}
			break;
		}
		if(c == '+' || c == '-') {
			switch(state) {
			case NSstart:
				state = NSsign;
				if(c == '-')
					rflag = !rflag;
				continue;
			case NSexp:
				state = NSexpsign;
				if(c == '-')
					expsign = 1;
				continue;
			}
			break;
		}
		if(c == '0') {
			switch(state) {
			case NSdigit:
				if(rflag)
					c = ~c;
				*kp++ = c;
				cl++;
				nzero++;
				dp++;
				state = NSdigit;
				continue;
			case NSfract:
				if(rflag)
					c = ~c;
				*kp++ = c;
				cl++;
				nzero++;
				state = NSfract;
				continue;
			case NSstart:
			case NSsign:
			case NSzero:
				state = NSzero;
				continue;
			case NSzerofract:
			case NSpoint:
				dp--;
				state = NSzerofract;
				continue;
			case NSexpsign:
			case NSexp:
			case NSexpdigit:
				exp = exp*10 + (c - '0');
				state = NSexpdigit;
				continue;
			}
			break;
		}
		if(c >= '1' && c <= '9') {
			switch(state) {
			case NSzero:
			case NSstart:
			case NSsign:
			case NSdigit:
				if(rflag)
					c = ~c;
				*kp++ = c;
				cl++;
				nzero = 0;
				dp++;
				state = NSdigit;
				continue;
			case NSzerofract:
			case NSpoint:
			case NSfract:
				if(rflag)
					c = ~c;
				*kp++ = c;
				cl++;
				nzero = 0;
				state = NSfract;
				continue;
			case NSexpsign:
			case NSexp:
			case NSexpdigit:
				exp = exp*10 + (c - '0');
				state = NSexpdigit;
				continue;
			}
			break;
		}
		if(c == '.') {
			switch(state) {
			case NSstart:
			case NSsign:
				state = NSpoint;
				continue;
			case NSzero:
				state = NSzerofract;
				continue;
			case NSdigit:
				state = NSfract;
				continue;
			}
			break;
		}
		if((f->flags & Gflag) && (c == 'e' || c == 'E')) {
			switch(state) {
			case NSdigit:
			case NSfract:
				state = NSexp;
				continue;
			}
			break;
		}
		break;
	}

	switch(state) {
	/*
	 * result is zero
	 */
	case NSstart:
	case NSsign:
	case NSzero:
	case NSzerofract:
	case NSpoint:
		kp = k->key + k->klen;
		k->klen += 2;
		kp[0] = 0x20;	/* between + and - */
		kp[1] = 0;
		return;
	/*
	 * result has exponent
	 */
	case NSexpsign:
	case NSexp:
	case NSexpdigit:
		if(expsign)
			exp = -exp;
		dp += exp;

	/*
	 * result is fixed point number
	 */
	case NSdigit:
	case NSfract:
		kp -= nzero;
		cl -= nzero;
		break;
	}

	/*
	 * end of number
	 */
	c = 0;
	if(rflag)
		c = ~c;
	*kp = c;

	/*
	 * sign and exponent
	 */
	c = 0x30;
	if(rflag) {
		c = 0x10;
		dp = ~dp;
	}
	kp = k->key + k->klen;
	kp[0] = c;
	kp[1] = (dp >> 8);
	kp[2] = dp;
	k->klen = cl+1;
}

void
dokey_m(Key *k, uchar *lp, uchar *lpe, Field *f)
{
	uchar *kp;
	Rune r, place[3];
	int c, cl, pc;
	int rflag;

	rflag = f->flags&Rflag;
	pc = 0;

	cl = k->klen;
	kp = k->key + cl;

	for(;;) {
		/*
		 * get the character
		 */
		if(lp >= lpe)
			break;
		c = *lp;
		if(c >= Runeself) {
			lp += chartorune(&r, (char*)lp);
			c = r;
		} else
			lp++;

		if(c < nelem(f->mapto)) {
			c = f->mapto[c];
			if(c == 0)
				continue;
		}
		place[pc++] = c;
		if(pc < 3)
			continue;
		for(c=11; c>=0; c--)
			if(memcmp(month[c], place, sizeof(place)) == 0)
				break;
		c += 10;
		if(rflag)
			c = ~c;
		*kp++ = c;
		cl++;
		break;
	}

	c = 0;
	if(rflag)
		c = ~c;
	*kp = c;
	k->klen = cl+1;
}

void
dokey_dfi(Key *k, uchar *lp, uchar *lpe, Field *f)
{
	uchar *kp;
	Rune r;
	int c, cl, n, rflag;

	cl = k->klen;
	kp = k->key + cl;
	rflag = f->flags & Rflag;

	for(;;) {
		/*
		 * get the character
		 */
		if(lp >= lpe)
			break;
		c = *lp;
		if(c >= Runeself) {
			lp += chartorune(&r, (char*)lp);
			c = r;
		} else
			lp++;

		/*
		 * do the various mappings.
		 * the common case is handled
		 * completely by the table.
		 */
		if(c != 0 && c < Runeself) {
			c = f->mapto[c];
			if(c) {
				*kp++ = c;
				cl++;
			}
			continue;
		}

		/*
		 * for characters out of range,
		 * the table does not do Rflag.
		 * ignore is based on mapto[255]
		 */
		if(c != 0 && c < nelem(f->mapto)) {
			c = f->mapto[c];
			if(c == 0)
				continue;
		} else
			if(f->mapto[nelem(f->mapto)-1] == 0)
				continue;

		/*
		 * put it in the key
		 */
		r = c;
		n = runetochar((char*)kp, &r);
		kp += n;
		cl += n;
		if(rflag)
			while(n > 0) {
				kp[-n] = ~kp[-n];
				n--;
			}
	}

	/*
	 * end of key
	 */
	k->klen = cl+1;
	if(rflag) {
		*kp = ~0;
		return;
	}
	*kp = 0;
}

void
dokey_r(Key *k, uchar *lp, uchar *lpe, Field *f)
{
	int cl, n;
	uchar *kp;

	USED(f);
	n = lpe - lp;
	if(n < 0)
		n = 0;
	cl = k->klen;
	kp = k->key + cl;
	k->klen = cl+n+1;

	lpe -= 3;
	while(lp < lpe) {
		kp[0] = ~lp[0];
		kp[1] = ~lp[1];
		kp[2] = ~lp[2];
		kp[3] = ~lp[3];
		kp += 4;
		lp += 4;
	}

	lpe += 3;
	while(lp < lpe)
		*kp++ = ~*lp++;
	*kp = ~0;
}

void
dokey_(Key *k, uchar *lp, uchar *lpe, Field *f)
{
	int n, cl;
	uchar *kp;

	USED(f);
	n = lpe - lp;
	if(n < 0)
		n = 0;
	cl = k->klen;
	kp = k->key + cl;
	k->klen = cl+n+1;
	memmove(kp, lp, n);
	kp[n] = 0;
}

void
buildkey(Line *l)
{
	Key *k;
	uchar *lp, *lpe;
	int ll, kl, cl, i, n;
	Field *f;

	ll = l->llen - 1;
	kl = 0;			/* allocated length */
	cl = 0;			/* current length */
	k = 0;

	for(i=1; i<=args.nfield; i++) {
		f = &args.field[i];
		lp = skip(l->line, f->beg1, f->beg2, f->flags&B1flag, 0);
		if(lp == 0)
			lp = l->line + ll;
		lpe = skip(l->line, f->end1, f->end2, f->flags&Bflag, 1);
		if(lpe == 0)
			lpe = l->line + ll;
		n = (lpe - lp) + 1;
		if(n <= 0)
			n = 1;
		if(cl+(n+4) > kl) {
			Key *nk;
			kl = cl+(n+4);
			/* amiport: safe realloc — preserve k on failure */
			nk = realloc(k, sizeof(Key) +
				(kl-1)*sizeof(k->key[0]));
			if(nk == 0) {
				free(k);
				nomem();
			}
			k = nk;
		}
		k->klen = cl;
		(*f->dokey)(k, lp, lpe, f);
		cl = k->klen;
	}

	/*
	 * global comparisons
	 */
	if(!(args.uflag && cl > 0)) {
		f = &args.field[0];
		if(cl+(ll+4) > kl) {
			Key *nk;
			kl = cl+(ll+4);
			/* amiport: safe realloc — preserve k on failure */
			nk = realloc(k, sizeof(Key) +
				(kl-1)*sizeof(k->key[0]));
			if(nk == 0) {
				free(k);
				nomem();
			}
			k = nk;
		}
		k->klen = cl;
		(*f->dokey)(k, l->line, l->line+ll, f);
		cl = k->klen;
	}

	l->key = k;
	k->klen = cl;

	if(args.vflag) {
		/* amiport: write(2, l->line, l->llen) replaced with fwrite(stderr) */
		fwrite(l->line, 1, l->llen, stderr);
		for(i=0; i<k->klen; i++) {
			/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
			fprintf(stderr, " %.2x", k->key[i]);
			if(k->key[i] == 0x00 || k->key[i] == 0xff)
				fprintf(stderr, "\n");
		}
	}
}

void
makemapm(Field *f)
{
	int i, c;

	for(i=0; i<nelem(f->mapto); i++) {
		c = 1;
		if(i == ' ' || i == '\t')
			c = 0;
		if(i >= 'a' && i <= 'z')
			c = i + ('A' - 'a');
		if(i >= 'A' && i <= 'Z')
			c = i;
		f->mapto[i] = c;
		if(args.vflag) {
			if((i & 15) == 0)
				fprintf(stderr, "\t");
			/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
			fprintf(stderr, " %.2x", c);
			if((i & 15) == 15)
				fprintf(stderr, "\n");
		}
	}
}

void
makemapd(Field *f)
{
	int i, j, c;

	for(i=0; i<nelem(f->mapto); i++) {
		c = i;
		if(f->flags & Iflag)
			if(c < 040 || c > 0176)
				c = -1;
		if((f->flags & Wflag) && c >= 0)
			if(c == ' ' || c == '\t')
				c = -1;
		if((f->flags & Dflag) && c >= 0)
			if(!(c == ' ' || c == '\t' ||
			    (c >= 'a' && c <= 'z') ||
			    (c >= 'A' && c <= 'Z') ||
			    (c >= '0' && c <= '9'))) {
				for(j=0; latinmap[j]; j+=3)
					if(c == latinmap[j+0] ||
					   c == latinmap[j+1])
						break;
				if(latinmap[j] == 0)
					c = -1;
			}
		if((f->flags & Fflag) && c >= 0) {
			if(c >= 'a' && c <= 'z')
				c += 'A' - 'a';
			for(j=0; latinmap[j]; j+=3)
				if(c == latinmap[j+0] ||
				   c == latinmap[j+1]) {
					c = latinmap[j+2];
					break;
				}
		}
		if((f->flags & Rflag) && c >= 0 && i > 0 && i < Runeself)
			c = ~c & 0xff;
		if(c < 0)
			c = 0;
		f->mapto[i] = c;
		if(args.vflag) {
			if((i & 15) == 0)
				fprintf(stderr, "\t");
			/* amiport: fprint(2,...) replaced with fprintf(stderr,...) */
			fprintf(stderr, " %.2x", c);
			if((i & 15) == 15)
				fprintf(stderr, "\n");
		}
	}
}

int	latinmap[] =
{
/*	lcase	ucase	fold	*/
	0xe0,	0xc0,	0x41,
	0xe1,	0xc1,	0x41,
	0xe2,	0xc2,	0x41,
	0xe4,	0xc4,	0x41,
	0xe3,	0xc3,	0x41,
	0xe5,	0xc5,	0x41,
	0xe8,	0xc8,	0x45,
	0xe9,	0xc9,	0x45,
	0xea,	0xca,	0x45,
	0xeb,	0xcb,	0x45,
	0xec,	0xcc,	0x49,
	0xed,	0xcd,	0x49,
	0xee,	0xce,	0x49,
	0xef,	0xcf,	0x49,
	0xf2,	0xd2,	0x4f,
	0xf3,	0xd3,	0x4f,
	0xf4,	0xd4,	0x4f,
	0xf6,	0xd6,	0x4f,
	0xf5,	0xd5,	0x4f,
	0xf8,	0xd8,	0x4f,
	0xf9,	0xd9,	0x55,
	0xfa,	0xda,	0x55,
	0xfb,	0xdb,	0x55,
	0xfc,	0xdc,	0x55,
	0xe6,	0xc6,	0x41,
	0xf0,	0xd0,	0x44,
	0xf1,	0xd1,	0x4e,
	0xfd,	0xdd,	0x59,
	0xe7,	0xc7,	0x43,
	0,
};

Rune LJAN[] = { 'J', 'A', 'N', 0 };
Rune LFEB[] = { 'F', 'E', 'B', 0 };
Rune LMAR[] = { 'M', 'A', 'R', 0 };
Rune LAPR[] = { 'A', 'P', 'R', 0 };
Rune LMAY[] = { 'M', 'A', 'Y', 0 };
Rune LJUN[] = { 'J', 'U', 'N', 0 };
Rune LJUL[] = { 'J', 'U', 'L', 0 };
Rune LAUG[] = { 'A', 'U', 'G', 0 };
Rune LSEP[] = { 'S', 'E', 'P', 0 };
Rune LOCT[] = { 'O', 'C', 'T', 0 };
Rune LNOV[] = { 'N', 'O', 'V', 0 };
Rune LDEC[] = { 'D', 'E', 'C', 0 };

Rune*	month[12] =
{
	LJAN,
	LFEB,
	LMAR,
	LAPR,
	LMAY,
	LJUN,
	LJUL,
	LAUG,
	LSEP,
	LOCT,
	LNOV,
	LDEC,
};

/************** radix sort ***********/

enum
{
	Threshold	= 14
};

void	rsort4(Key***, ulong, int);
void	bsort4(Key***, ulong, int);

void
sort4(void *a, ulong n)
{
	if(n > Threshold)
		rsort4((Key***)a, n, 0);
	else
		bsort4((Key***)a, n, 0);
}

void
rsort4(Key ***a, ulong n, int b)
{
	Key ***ea, ***t, ***u, **t1, **u1, *k;
	/* amiport: part[] and clist[] made static to avoid ~3100 bytes per
	 * stack frame in recursive rsort4(). On real AmigaOS, dos.library adds
	 * 2-4KB hidden stack depth (crash-patterns #10). With recursion depth
	 * proportional to key length, stack-allocated arrays would overflow
	 * __stack=65536 on lines >21 chars. Safe: single-threaded, not reentrant. */
	static Key ***part[257];
	static long count[257];
	static long clist[257+257];
	long *cp, *cp1;
	int c, lowc, higc;

	lowc = 256;
	higc = 0;
	ea = a+n;
	for(t=a; t<ea; t++) {
		k = **t;
		n = k->klen;
		if(b >= n) {
			count[256]++;
			continue;
		}
		c = k->key[b];
		n = count[c]++;
		if(n == 0) {
			if(c < lowc)
				lowc = c;
			if(c > higc)
				higc = c;
		}
	}

	t = a;
	n = count[256];
	clist[0] = n;
	part[256] = t;
	t += n;

	cp1 = clist+1;
	cp = count+lowc;
	for(c=lowc; c<=higc; c++,cp++) {
		n = *cp;
		if(n) {
			cp1[0] = n;
			cp1[1] = c;
			cp1 += 2;
			part[c] = t;
			t += n;
		}
	}
	*cp1 = 0;

	for(cp1=clist+1; cp1[0]; cp1+=2) {
		c = cp1[1];
		cp = count+c;
		while(*cp) {
			t1 = *part[c];
			for(;;) {
				k = *t1;
				n = 256;
				if(b < k->klen)
					n = k->key[b];
				u = part[n]++;
				count[n]--;
				u1 = *u;
				*u = t1;
				if(n == c)
					break;
				t1 = u1;
			}
		}
	}

	b++;
	t = a + clist[0];
	count[256] = 0;
	for(cp1=clist+1; (n=cp1[0]) != 0; cp1+=2) {
		if(n > Threshold)
			rsort4(t, n, b);
		else
		if(n > 1)
			bsort4(t, n, b);
		t += n;
	}
}

void
bsort4(Key ***a, ulong n, int b)
{
	Key ***i, ***j, ***k, ***l, **t;
	Key *ka, *kb;
	int n1, n2;

	l = a+n;
	j = a;

loop:
	i = j;
	j++;
	if(j >= l)
		return;

	ka = **i;
	kb = **j;
	n1 = ka->klen - b;
	n2 = kb->klen - b;
	if(n1 > n2)
		n1 = n2;
	if(n1 <= 0)
		goto loop;
	n2 = ka->key[b] - kb->key[b];
	if(n2 == 0)
		n2 = memcmp(ka->key+b, kb->key+b, n1);
	if(n2 <= 0)
		goto loop;

	for(;;) {
		k = i+1;

		t = *k;
		*k = *i;
		*i = t;

		if(i <= a)
			goto loop;

		i--;
		ka = **i;
		kb = *t;
		n1 = ka->klen - b;
		n2 = kb->klen - b;
		if(n1 > n2)
			n1 = n2;
		if(n1 <= 0)
			goto loop;
		n2 = ka->key[b] - kb->key[b];
		if(n2 == 0)
			n2 = memcmp(ka->key+b, kb->key+b, n1);
		if(n2 <= 0)
			goto loop;
	}
}
