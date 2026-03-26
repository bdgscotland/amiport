/*	$OpenBSD: test.c,v 1.23 2025/03/24 20:15:08 millert Exp $	*/
/*	$NetBSD: test.c,v 1.15 1995/03/21 07:04:06 cgd Exp $	*/

/*
 * test(1); version 7-like  --  author Erik Baalbergen
 * modified by Eric Gisin to be used as built-in.
 * modified by Arnold Robbins to add SVR3 compatibility
 * (-x -c -b -p -u -g -k) plus Korn's -L -nt -ot -ef and new -S (socket).
 * modified by J.T. Conklin for NetBSD.
 *
 * This program is in the Public Domain.
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: test 1.23 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

/* amiport: suppress wildcard expansion -- program takes pattern/expression args */
int __nowild = 1;

#include <sys/types.h>
/* amiport: replaced <sys/stat.h> with amiport shim */
#include <amiport/sys/stat.h>
/* amiport: replaced <unistd.h> with amiport shim (provides access(), isatty()) */
#include <amiport/unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport shim (activates exit() -> amiport_exit()) */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <err.h> with amiport shim */
#include <amiport/err.h>
/* amiport: strlcpy/strtonum shims */
#include <amiport/string.h>
/* amiport: __progname provided by glob.h shim */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/*
 * amiport: __dead is an OpenBSD attribute for noreturn functions.
 * GCC provides __attribute__((noreturn)) instead.
 */
#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

/*
 * amiport: geteuid()/getegid() -- AmigaOS is single-user, stub as 0.
 * -O (file owned by effective user) and -G (file owned by effective group)
 * will always compare st_uid/st_gid with 0. AmigaOS stat returns 0 for
 * both uid and gid, so ownership checks return true for all files.
 */
#define geteuid() (0)
#define getegid() (0)

/*
 * amiport: S_IFCHR, S_IFBLK, S_IFIFO, S_IFSOCK -- AmigaOS 3.x FFS/OFS has no
 * character devices, block devices, FIFOs, or sockets as filesystem entries.
 * Define the mode bits so the code compiles; the stat() shim will never
 * return these file types, so -c, -b, -p, -S always return false.
 */
#ifndef S_IFCHR
#define S_IFCHR  0020000
#endif
#ifndef S_IFBLK
#define S_IFBLK  0060000
#endif
#ifndef S_IFIFO
#define S_IFIFO  0010000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

/*
 * amiport: S_ISUID, S_ISGID, S_ISVTX -- AmigaOS has no setuid/setgid/sticky bits.
 * Define them so the code compiles; the stat() shim never sets these bits,
 * so -u, -g, -k always return false.
 */
#ifndef S_ISUID
#define S_ISUID  0004000
#endif
#ifndef S_ISGID
#define S_ISGID  0002000
#endif
#ifndef S_ISVTX
#define S_ISVTX  0001000
#endif

/*
 * amiport: S_IFLNK -- AmigaOS FFS/OFS/SFS has no symbolic links in the POSIX
 * sense. Define the bit so the code compiles; lstat() is aliased to stat()
 * by the shim and will never set S_IFLNK, so -L/-h always return false.
 */
#ifndef S_IFLNK
#define S_IFLNK  0120000
#endif

/*
 * amiport: st_uid / st_gid -- amiport_stat struct does not provide these
 * fields (AmigaOS is single-user). Define accessor macros that always
 * return 0, matching geteuid()/getegid() stubs above.
 *
 * amiport: st_mtim -- amiport_stat struct provides st_mtime (Unix timestamp)
 * but not st_mtim (struct timespec). Provide a compatibility wrapper.
 *
 * amiport: timespeccmp -- OpenBSD macro comparing two struct timespec values.
 * We implement it using the scalar st_mtime field from amiport_stat.
 */

/*
 * Augmented stat wrapper: wraps amiport_stat and adds the fields that
 * amiport_stat omits. We define our own struct and a wrapper function so
 * the rest of the code can use the standard field names.
 *
 * Fields provided by amiport_stat:
 *   st_mode, st_size, st_mtime, st_dev, st_ino
 *
 * Fields synthesised here (always 0 on AmigaOS):
 *   st_uid, st_gid
 *
 * timespeccmp replacement:
 *   We compare using st_mtime (scalar seconds). Sub-second precision is not
 *   available from AmigaOS Lock()/Examine(), so -nt/-ot have 1-second
 *   granularity. This is documented as a known limitation.
 */

/* amiport: full stat struct with synthesised POSIX fields */
struct full_stat {
    unsigned long st_mode;
    long          st_size;
    unsigned long st_mtime;  /* modification time (Unix seconds, ~1s precision) */
    unsigned long st_dev;
    unsigned long st_ino;
    int           st_uid;    /* always 0 (AmigaOS single-user) */
    int           st_gid;    /* always 0 (AmigaOS single-user) */
};

/*
 * amiport: full_stat_get() -- wraps amiport_stat() and fills the full_stat
 * struct including synthesised uid/gid fields.
 */
static int
full_stat_get(const char *path, struct full_stat *out)
{
    struct amiport_stat s;
    int r;
    r = amiport_stat(path, &s);
    if (r != 0)
        return r;
    out->st_mode  = s.st_mode;
    out->st_size  = s.st_size;
    out->st_mtime = s.st_mtime;
    out->st_dev   = s.st_dev;
    out->st_ino   = s.st_ino;
    out->st_uid   = 0;
    out->st_gid   = 0;
    return 0;
}

/*
 * amiport: timespeccmp replacement using scalar mtime.
 * Only > and < comparisons are used by newerf/olderf.
 * CMP must be a binary operator: >, <, ==, !=, >=, <=
 * We emulate by comparing st_mtime (unsigned long seconds).
 */
#define timespeccmp(a, b, CMP) ((a)->st_mtime CMP (b)->st_mtime)

/* test(1) accepts the following grammar:
	oexpr	::= aexpr | aexpr "-o" oexpr ;
	aexpr	::= nexpr | nexpr "-a" aexpr ;
	nexpr	::= primary | "!" primary
	primary	::= unary-operator operand
		| operand binary-operator operand
		| operand
		| "(" oexpr ")"
		;
	unary-operator ::= "-r"|"-w"|"-x"|"-f"|"-d"|"-c"|"-b"|"-p"|
		"-u"|"-g"|"-k"|"-s"|"-t"|"-z"|"-n"|"-o"|"-O"|"-G"|"-L"|"-S";

	binary-operator ::= "="|"!="|"<"|">"|"-eq"|"-ne"|"-ge"|"-gt"|
			"-le"|"-lt"|"-nt"|"-ot"|"-ef";
	operand ::= <any legal UNIX file name>
*/

enum token {
	EOI,
	FILRD,
	FILWR,
	FILEX,
	FILEXIST,
	FILREG,
	FILDIR,
	FILCDEV,
	FILBDEV,
	FILFIFO,
	FILSOCK,
	FILSYM,
	FILGZ,
	FILTT,
	FILSUID,
	FILSGID,
	FILSTCK,
	FILNT,
	FILOT,
	FILEQ,
	FILUID,
	FILGID,
	STREZ,
	STRNZ,
	STREQ,
	STRNE,
	STRLT,
	STRGT,
	INTEQ,
	INTNE,
	INTGE,
	INTGT,
	INTLE,
	INTLT,
	UNOT,
	BAND,
	BOR,
	LPAREN,
	RPAREN,
	OPERAND
};

enum token_types {
	UNOP,
	BINOP,
	BUNOP,
	BBINOP,
	PAREN
};

struct t_op {
	const char *op_text;
	short op_num, op_type;
} const ops [] = {
	{"-r",	FILRD,	UNOP},
	{"-w",	FILWR,	UNOP},
	{"-x",	FILEX,	UNOP},
	{"-e",	FILEXIST,UNOP},
	{"-f",	FILREG,	UNOP},
	{"-d",	FILDIR,	UNOP},
	{"-c",	FILCDEV,UNOP},
	{"-b",	FILBDEV,UNOP},
	{"-p",	FILFIFO,UNOP},
	{"-u",	FILSUID,UNOP},
	{"-g",	FILSGID,UNOP},
	{"-k",	FILSTCK,UNOP},
	{"-s",	FILGZ,	UNOP},
	{"-t",	FILTT,	UNOP},
	{"-z",	STREZ,	UNOP},
	{"-n",	STRNZ,	UNOP},
	{"-h",	FILSYM,	UNOP},
	{"-O",	FILUID,	UNOP},
	{"-G",	FILGID,	UNOP},
	{"-L",	FILSYM,	UNOP},
	{"-S",	FILSOCK,UNOP},
	{"=",	STREQ,	BINOP},
	{"!=",	STRNE,	BINOP},
	{"<",	STRLT,	BINOP},
	{">",	STRGT,	BINOP},
	{"-eq",	INTEQ,	BINOP},
	{"-ne",	INTNE,	BINOP},
	{"-ge",	INTGE,	BINOP},
	{"-gt",	INTGT,	BINOP},
	{"-le",	INTLE,	BINOP},
	{"-lt",	INTLT,	BINOP},
	{"-nt",	FILNT,	BINOP},
	{"-ot",	FILOT,	BINOP},
	{"-ef",	FILEQ,	BINOP},
	{"!",	UNOT,	BUNOP},
	{"-a",	BAND,	BBINOP},
	{"-o",	BOR,	BBINOP},
	{"(",	LPAREN,	PAREN},
	{")",	RPAREN,	PAREN},
	{0,	0,	0}
};

char **t_wp;
struct t_op const *t_wp_op;

static enum token t_lex(char *);
static enum token_types t_lex_type(char *);
static int oexpr(enum token n);
static int aexpr(enum token n);
static int nexpr(enum token n);
static int binop(void);
static int primary(enum token n);
static const char *getnstr(const char *, int *, size_t *);
static int intcmp(const char *, const char *);
static int filstat(char *nm, enum token mode);
static int getn(const char *s);
static int newerf(const char *, const char *);
static int olderf(const char *, const char *);
static int equalf(const char *, const char *);
static __dead void syntax(const char *op, char *msg);

int
main(int argc, char *argv[])
{
	extern char *__progname;
	int	res;

	/* amiport: pledge not available on AmigaOS -- stubbed to (0) */
	if (pledge("stdio rpath", NULL) == -1)
		err(20, "pledge"); /* amiport: exit 2 -> 20 (RETURN_FAIL) */

	if (strcmp(__progname, "[") == 0) {
		if (strcmp(argv[--argc], "]"))
			errx(20, "missing ]"); /* amiport: exit 2 -> 20 (RETURN_FAIL) */
		argv[argc] = NULL;
	}

	/* Implement special cases from POSIX.2, section 4.62.4 */
	switch (argc) {
	case 1:
		return 1;
	case 2:
		return (*argv[1] == '\0');
	case 3:
		if (argv[1][0] == '!' && argv[1][1] == '\0') {
			return !(*argv[2] == '\0');
		}
		break;
	case 4:
		if (argv[1][0] != '!' || argv[1][1] != '\0') {
			if (t_lex(argv[2]),
			    t_wp_op && t_wp_op->op_type == BINOP) {
				t_wp = &argv[1];
				return (binop() == 0);
			}
		}
		break;
	case 5:
		if (argv[1][0] == '!' && argv[1][1] == '\0') {
			if (t_lex(argv[3]),
			    t_wp_op && t_wp_op->op_type == BINOP) {
				t_wp = &argv[2];
				return !(binop() == 0);
			}
		}
		break;
	}

	t_wp = &argv[1];
	res = !oexpr(t_lex(*t_wp));

	if (*t_wp != NULL && *++t_wp != NULL)
		syntax(*t_wp, "unknown operand");

	return res;
}

static __dead void
syntax(const char *op, char *msg)
{
	if (op && *op)
		errx(20, "%s: %s", op, msg); /* amiport: exit 2 -> 20 (RETURN_FAIL) */
	else
		errx(20, "%s", msg); /* amiport: exit 2 -> 20 (RETURN_FAIL) */
}

static int
oexpr(enum token n)
{
	int res;

	res = aexpr(n);
	if (t_lex(*++t_wp) == BOR)
		return oexpr(t_lex(*++t_wp)) || res;
	t_wp--;
	return res;
}

static int
aexpr(enum token n)
{
	int res;

	res = nexpr(n);
	if (t_lex(*++t_wp) == BAND)
		return aexpr(t_lex(*++t_wp)) && res;
	t_wp--;
	return res;
}

static int
nexpr(enum token n)
{
	if (n == UNOT)
		return !nexpr(t_lex(*++t_wp));
	return primary(n);
}

static int
primary(enum token n)
{
	int res;

	if (n == EOI)
		syntax(NULL, "argument expected");
	if (n == LPAREN) {
		res = oexpr(t_lex(*++t_wp));
		if (t_lex(*++t_wp) != RPAREN)
			syntax(NULL, "closing paren expected");
		return res;
	}
	/*
	 * We need this, if not binary operations with more than 4
	 * arguments will always fall into unary.
	 */
	if(t_lex_type(t_wp[1]) == BINOP) {
		t_lex(t_wp[1]);
		if (t_wp_op && t_wp_op->op_type == BINOP)
			return binop();
	}

	if (t_wp_op && t_wp_op->op_type == UNOP) {
		/* unary expression */
		if (*++t_wp == NULL)
			syntax(t_wp_op->op_text, "argument expected");
		switch (n) {
		case STREZ:
			return strlen(*t_wp) == 0;
		case STRNZ:
			return strlen(*t_wp) != 0;
		case FILTT:
			/* amiport: isatty() -- amiport_isatty() checks amiport fd table;
			 * for fd 0/1/2, use IsInteractive() directly instead.
			 * amiport_isatty() returns 0 for libnix stdio fds, but isatty()
			 * from libnix should work for the common -t 1 case. */
			return isatty(getn(*t_wp));
		default:
			return filstat(*t_wp, n);
		}
	}

	return strlen(*t_wp) > 0;
}

static const char *
getnstr(const char *s, int *signum, size_t *len)
{
	const char *p, *start;

	/* skip leading whitespaces */
	p = s;
	while (isspace((unsigned char)*p))
		p++;

	/* accept optional sign */
	if (*p == '-') {
		*signum = -1;
		p++;
	} else {
		*signum = 1;
		if (*p == '+')
			p++;
	}

	/* skip leading zeros */
	while (*p == '0' && isdigit((unsigned char)p[1]))
		p++;

	/* turn 0 always positive */
	if (*p == '0')
		*signum = 1;

	start = p;
	while (isdigit((unsigned char)*p))
		p++;
	*len = p - start;

	/* allow trailing whitespaces */
	while (isspace((unsigned char)*p))
		p++;

	/* validate number */
	if (*p != '\0' || *start == '\0')
		errx(20, "%s: invalid", s); /* amiport: exit 2 -> 20 (RETURN_FAIL) */

	return start;
}

static int
intcmp(const char *opnd1, const char *opnd2)
{
	const char *p1, *p2;
	size_t len1, len2;
	int c, sig1, sig2;

	p1 = getnstr(opnd1, &sig1, &len1);
	p2 = getnstr(opnd2, &sig2, &len2);

	if (sig1 != sig2)
		c = sig1;
	else if (len1 != len2)
		c = (len1 < len2) ? -sig1 : sig1;
	else
		c = strncmp(p1, p2, len1) * sig1;

	return c;
}

static int
binop(void)
{
	const char *opnd1, *opnd2;
	struct t_op const *op;

	opnd1 = *t_wp;
	(void) t_lex(*++t_wp);
	op = t_wp_op;

	if ((opnd2 = *++t_wp) == NULL)
		syntax(op->op_text, "argument expected");

	switch (op->op_num) {
	case STREQ:
		return strcmp(opnd1, opnd2) == 0;
	case STRNE:
		return strcmp(opnd1, opnd2) != 0;
	case STRLT:
		return strcmp(opnd1, opnd2) < 0;
	case STRGT:
		return strcmp(opnd1, opnd2) > 0;
	case INTEQ:
		return intcmp(opnd1, opnd2) == 0;
	case INTNE:
		return intcmp(opnd1, opnd2) != 0;
	case INTGE:
		return intcmp(opnd1, opnd2) >= 0;
	case INTGT:
		return intcmp(opnd1, opnd2) > 0;
	case INTLE:
		return intcmp(opnd1, opnd2) <= 0;
	case INTLT:
		return intcmp(opnd1, opnd2) < 0;
	case FILNT:
		return newerf(opnd1, opnd2);
	case FILOT:
		return olderf(opnd1, opnd2);
	case FILEQ:
		return equalf(opnd1, opnd2);
	}

	syntax(op->op_text, "not a binary operator");
}

static enum token_types
t_lex_type(char *s)
{
	struct t_op const *op = ops;

	if (s == NULL)
		return -1;

	while (op->op_text) {
		if (strcmp(s, op->op_text) == 0)
			return op->op_type;
		op++;
	}
	return -1;
}

static int
filstat(char *nm, enum token mode)
{
	struct amiport_stat s;
	/* amiport: mode_t not defined in amiport shim -- use unsigned long */
	unsigned long i;

	switch (mode) {
	case FILRD:
		/* amiport: replaced access() with amiport_access() via macro in unistd.h */
		return access(nm, R_OK) == 0;
	case FILWR:
		return access(nm, W_OK) == 0;
	case FILEX:
		return access(nm, X_OK) == 0;
	case FILEXIST:
		return access(nm, F_OK) == 0;
	default:
		break;
	}

	if (mode == FILSYM) {
		/*
		 * amiport: lstat() aliased to stat() -- AmigaOS FFS/OFS/SFS has no
		 * POSIX symlinks. -L and -h always return false because S_IFLNK is
		 * never set in the mode returned by amiport_stat().
		 */
		if (lstat(nm, &s) == 0) {
			i = S_IFLNK;
			goto filetype;
		}
		return 0;
	}

	/* amiport: replaced stat() with amiport_stat() via macro in sys/stat.h */
	if (stat(nm, &s) != 0)
		return 0;

	switch (mode) {
	case FILREG:
		i = S_IFREG;
		goto filetype;
	case FILDIR:
		i = S_IFDIR;
		goto filetype;
	case FILCDEV:
		/*
		 * amiport: S_IFCHR -- no character devices on AmigaOS FFS/OFS/SFS.
		 * amiport_stat() never sets S_IFCHR; -c always returns false.
		 */
		i = S_IFCHR;
		goto filetype;
	case FILBDEV:
		/*
		 * amiport: S_IFBLK -- no block devices on AmigaOS FFS/OFS/SFS.
		 * amiport_stat() never sets S_IFBLK; -b always returns false.
		 */
		i = S_IFBLK;
		goto filetype;
	case FILFIFO:
		/*
		 * amiport: S_IFIFO -- no FIFOs on AmigaOS FFS/OFS/SFS.
		 * amiport_stat() never sets S_IFIFO; -p always returns false.
		 */
		i = S_IFIFO;
		goto filetype;
	case FILSOCK:
		/*
		 * amiport: S_IFSOCK -- no UNIX domain sockets on AmigaOS.
		 * amiport_stat() never sets S_IFSOCK; -S always returns false.
		 */
		i = S_IFSOCK;
		goto filetype;
	case FILSUID:
		/*
		 * amiport: S_ISUID -- no setuid bit on AmigaOS.
		 * amiport_stat() never sets S_ISUID; -u always returns false.
		 */
		i = S_ISUID;
		goto filebit;
	case FILSGID:
		/*
		 * amiport: S_ISGID -- no setgid bit on AmigaOS.
		 * amiport_stat() never sets S_ISGID; -g always returns false.
		 */
		i = S_ISGID;
		goto filebit;
	case FILSTCK:
		/*
		 * amiport: S_ISVTX -- no sticky bit on AmigaOS.
		 * amiport_stat() never sets S_ISVTX; -k always returns false.
		 */
		i = S_ISVTX;
		goto filebit;
	case FILGZ:
		return s.st_size > 0L;
	case FILUID:
		/*
		 * amiport: AmigaOS is single-user. geteuid() is stubbed to 0 and
		 * all files have owner 0. -O always returns true for existing files.
		 * (struct amiport_stat has no st_uid field; evaluate directly.)
		 */
		return (geteuid() == 0);
	case FILGID:
		/*
		 * amiport: AmigaOS is single-user. getegid() is stubbed to 0 and
		 * all files have group 0. -G always returns true for existing files.
		 * (struct amiport_stat has no st_gid field; evaluate directly.)
		 */
		return (getegid() == 0);
	default:
		return 1;
	}

filetype:
	return ((s.st_mode & S_IFMT) == i);

filebit:
	return ((s.st_mode & i) != 0);
}

static enum token
t_lex(char *s)
{
	struct t_op const *op = ops;

	if (s == 0) {
		t_wp_op = NULL;
		return EOI;
	}
	while (op->op_text) {
		if (strcmp(s, op->op_text) == 0) {
			t_wp_op = op;
			return op->op_num;
		}
		op++;
	}
	t_wp_op = NULL;
	return OPERAND;
}

/* atoi with error detection */
static int
getn(const char *s)
{
	char buf[32];
	const char *errstr, *p;
	size_t len;
	int r, sig;

	p = getnstr(s, &sig, &len);
	if (sig != 1)
		errstr = "too small";
	else if (len >= sizeof(buf))
		errstr = "too large";
	else {
		/* amiport: strlcpy() via amiport/string.h shim */
		strlcpy(buf, p, sizeof(buf));
		buf[len] = '\0';
		/* amiport: strtonum() via amiport/err.h shim */
		r = strtonum(buf, 0, INT_MAX, &errstr);
	}

	if (errstr != NULL)
		errx(20, "%s: %s", s, errstr); /* amiport: exit 2 -> 20 (RETURN_FAIL) */

	return r;
}

static int
newerf(const char *f1, const char *f2)
{
	struct full_stat b1, b2;

	/*
	 * amiport: replaced struct stat with struct full_stat (includes synthesised
	 * uid/gid fields); replaced stat() calls with full_stat_get() wrapper.
	 * amiport: timespeccmp() replaced with scalar st_mtime comparison --
	 * AmigaOS provides ~1-second timestamp granularity (no sub-second precision).
	 * -nt and -ot comparisons have 1-second granularity on AmigaOS.
	 */
	return (full_stat_get(f1, &b1) == 0 &&
	    full_stat_get(f2, &b2) == 0 &&
	    timespeccmp(&b1, &b2, >));
}

static int
olderf(const char *f1, const char *f2)
{
	struct full_stat b1, b2;

	/* amiport: see newerf() comment */
	return (full_stat_get(f1, &b1) == 0 &&
	    full_stat_get(f2, &b2) == 0 &&
	    timespeccmp(&b1, &b2, <));
}

static int
equalf(const char *f1, const char *f2)
{
	struct amiport_stat b1, b2;

	/*
	 * amiport: -ef (equal file) -- checks st_dev and st_ino.
	 * amiport_stat() provides both fields from Lock()+Examine().
	 * st_dev is derived from the volume lock; st_ino from fib_DiskKey.
	 * This correctly identifies hard links on AmigaOS (same file, same inode).
	 */
	return (stat(f1, &b1) == 0 &&
	    stat(f2, &b2) == 0 &&
	    b1.st_dev == b2.st_dev &&
	    b1.st_ino == b2.st_ino);
}
