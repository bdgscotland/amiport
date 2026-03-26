/*	$OpenBSD: expr.c,v 1.28 2022/01/28 05:15:05 guenther Exp $	*/
/*	$NetBSD: expr.c,v 1.3.6.1 1996/06/04 20:41:47 cgd Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

/* amiport: Amiga version string */
static const char *verstag __attribute__((used)) = "$VER: expr 1.28 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: suppress wildcard expansion -- expr takes expression arguments,
 * not filenames. Without this, operands like "*.c" would be glob-expanded. */
int __nowild = 1;

#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates
 * exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
/* amiport: replaced <regex.h> with <amiport-emu/regex.h> -- Tier 2 emulation
 * via Henry Spencer's NFA regex engine. No locale collation, no [:class:]
 * character classes, max 9 subexpressions, backtracking NFA semantics. */
/* amiport-emu: regex emulated -- no locale collation, no [:class:],
 * max 9 groups, backtracking NFA */
#include <amiport-emu/regex.h>
/* amiport: err/warn/strtonum via shim -- provides err(), errx(), strtonum() */
#include <amiport/err.h>
/* amiport: asprintf() via shim */
#include <amiport/stdio_ext.h>
/* amiport: amiport_expand_argv / amiport_free_argv / __progname */
#include <amiport/glob.h>

/* amiport: removed <stdint.h> -- C99 header not available in libnix.
 * Define int64_t and its limits locally using long long (64-bit on 68k). */
typedef long long int64_t;
/* amiport: INT64_MIN/INT64_MAX -- defined locally, <stdint.h> not available */
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX  9223372036854775807LL
#endif

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: __dead is an OpenBSD extension for noreturn functions */
#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

struct val	*make_int(int64_t);
struct val	*make_str(char *);
void		 free_value(struct val *);
int		 is_integer(struct val *, int64_t *, const char **);
int		 to_integer(struct val *, const char **);
void		 to_string(struct val *);
int		 is_zero_or_null(struct val *);
void		 nexttoken(int);
__dead void	 error(void);
struct val	*eval6(void);
struct val	*eval5(void);
struct val	*eval4(void);
struct val	*eval3(void);
struct val	*eval2(void);
struct val	*eval1(void);
struct val	*eval0(void);

enum token {
	OR, AND, EQ, LT, GT, ADD, SUB, MUL, DIV, MOD, MATCH, RP, LP,
	NE, LE, GE, OPERAND, EOI
};

struct val {
	enum {
		integer,
		string
	} type;

	union {
		char	       *s;
		int64_t		i;
	} u;
};

enum token	token;
struct val     *tokval;
char	      **av;

struct val *
make_int(int64_t i)
{
	struct val     *vp;

	vp = malloc(sizeof(*vp));
	if (vp == NULL) {
		/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
		err(10, NULL);
	}
	vp->type = integer;
	vp->u.i = i;
	return vp;
}


struct val *
make_str(char *s)
{
	struct val     *vp;

	vp = malloc(sizeof(*vp));
	if (vp == NULL || ((vp->u.s = strdup(s)) == NULL)) {
		/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
		err(10, NULL);
	}
	vp->type = string;
	return vp;
}


void
free_value(struct val *vp)
{
	if (vp->type == string)
		free(vp->u.s);
	free(vp);
}


/* determine if vp is an integer; if so, return its value in *r */
int
is_integer(struct val *vp, int64_t *r, const char **errstr)
{
	const char *errstrp;

	if (errstr == NULL)
		errstr = &errstrp;
	*errstr = NULL;

	if (vp->type == integer) {
		*r = vp->u.i;
		return 1;
	}

	/*
	 * POSIX.2 defines an "integer" as an optional unary minus
	 * followed by digits. Other representations are unspecified,
	 * which means that strtonum(3) is a viable option here.
	 */
	*r = strtonum(vp->u.s, INT64_MIN, INT64_MAX, errstr);
	return *errstr == NULL;
}


/* coerce to vp to an integer */
int
to_integer(struct val *vp, const char **errstr)
{
	int64_t		r;

	if (errstr != NULL)
		*errstr = NULL;

	if (vp->type == integer)
		return 1;

	if (is_integer(vp, &r, errstr)) {
		free(vp->u.s);
		vp->u.i = r;
		vp->type = integer;
		return 1;
	}

	return 0;
}


/* coerce to vp to an string */
void
to_string(struct val *vp)
{
	char	       *tmp;

	if (vp->type == string)
		return;

	/* amiport: asprintf() via <amiport/stdio_ext.h> shim */
	if (asprintf(&tmp, "%lld", vp->u.i) == -1)
		/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
		err(10, NULL);

	vp->type = string;
	vp->u.s = tmp;
}

int
is_zero_or_null(struct val *vp)
{
	if (vp->type == integer)
		return vp->u.i == 0;
	else
		return *vp->u.s == 0 || (to_integer(vp, NULL) && vp->u.i == 0);
}

void
nexttoken(int pat)
{
	char	       *p;

	if ((p = *av) == NULL) {
		token = EOI;
		return;
	}
	av++;


	if (pat == 0 && p[0] != '\0') {
		if (p[1] == '\0') {
			const char     *x = "|&=<>+-*/%:()";
			char	       *i;	/* index */

			if ((i = strchr(x, *p)) != NULL) {
				token = i - x;
				return;
			}
		} else if (p[1] == '=' && p[2] == '\0') {
			switch (*p) {
			case '<':
				token = LE;
				return;
			case '>':
				token = GE;
				return;
			case '!':
				token = NE;
				return;
			}
		}
	}
	tokval = make_str(p);
	token = OPERAND;
	return;
}

__dead void
error(void)
{
	/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
	errx(10, "syntax error");
}

struct val *
eval6(void)
{
	struct val     *v;

	if (token == OPERAND) {
		nexttoken(0);
		return tokval;
	} else if (token == RP) {
		nexttoken(0);
		v = eval0();
		if (token != LP)
			error();
		nexttoken(0);
		return v;
	} else
		error();
}

/* Parse and evaluate match (regex) expressions */
struct val *
eval5(void)
{
	/* amiport-emu: regex_t and regmatch_t mapped via <amiport-emu/regex.h>
	 * macros to amiport_emu_regex_t / amiport_emu_regmatch_t */
	regex_t		rp;
	regmatch_t	rm[2];
	char		errbuf[256];
	int		eval;
	struct val     *l, *r;
	struct val     *v;

	l = eval6();
	while (token == MATCH) {
		nexttoken(1);
		r = eval6();

		/* coerce to both arguments to strings */
		to_string(l);
		to_string(r);

		/* compile regular expression */
		/* amiport-emu: regcomp mapped to amiport_emu_regcomp via macro */
		if ((eval = regcomp(&rp, r->u.s, 0)) != 0) {
			/* amiport-emu: regerror mapped to amiport_emu_regerror via macro */
			regerror(eval, &rp, errbuf, sizeof(errbuf));
			/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
			errx(10, "%s", errbuf);
		}

		/* compare string against pattern --  remember that patterns
		   are anchored to the beginning of the line */
		/* amiport-emu: regexec mapped to amiport_emu_regexec via macro */
		if (regexec(&rp, l->u.s, 2, rm, 0) == 0 && rm[0].rm_so == 0) {
			if (rm[1].rm_so >= 0) {
				*(l->u.s + rm[1].rm_eo) = '\0';
				v = make_str(l->u.s + rm[1].rm_so);

			} else {
				v = make_int(rm[0].rm_eo - rm[0].rm_so);
			}
		} else {
			if (rp.re_nsub == 0) {
				v = make_int(0);
			} else {
				v = make_str("");
			}
		}

		/* free arguments and pattern buffer */
		free_value(l);
		free_value(r);
		/* amiport-emu: regfree mapped to amiport_emu_regfree via macro */
		regfree(&rp);

		l = v;
	}

	return l;
}

/* Parse and evaluate multiplication and division expressions */
struct val *
eval4(void)
{
	const char	*errstr;
	struct val	*l, *r;
	enum token	 op;
	volatile int64_t res;

	l = eval5();
	while ((op = token) == MUL || op == DIV || op == MOD) {
		nexttoken(0);
		r = eval5();

		if (!to_integer(l, &errstr))
			/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
			errx(10, "number \"%s\" is %s", l->u.s, errstr);
		if (!to_integer(r, &errstr))
			/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
			errx(10, "number \"%s\" is %s", r->u.s, errstr);

		if (op == MUL) {
			res = l->u.i * r->u.i;
			if (r->u.i != 0 && l->u.i != res / r->u.i)
				/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
				errx(10, "overflow");
			l->u.i = res;
		} else {
			if (r->u.i == 0) {
				/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
				errx(10, "division by zero");
			}
			if (op == DIV) {
				if (l->u.i != INT64_MIN || r->u.i != -1)
					l->u.i /= r->u.i;
				else
					/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
					errx(10, "overflow");
			} else {
				if (l->u.i != INT64_MIN || r->u.i != -1)
					l->u.i %= r->u.i;
				else
					l->u.i = 0;
			}
		}

		free_value(r);
	}

	return l;
}

/* Parse and evaluate addition and subtraction expressions */
struct val *
eval3(void)
{
	const char	*errstr;
	struct val	*l, *r;
	enum token	 op;
	volatile int64_t res;

	l = eval4();
	while ((op = token) == ADD || op == SUB) {
		nexttoken(0);
		r = eval4();

		if (!to_integer(l, &errstr))
			/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
			errx(10, "number \"%s\" is %s", l->u.s, errstr);
		if (!to_integer(r, &errstr))
			/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
			errx(10, "number \"%s\" is %s", r->u.s, errstr);

		if (op == ADD) {
			res = l->u.i + r->u.i;
			if ((l->u.i > 0 && r->u.i > 0 && res <= 0) ||
			    (l->u.i < 0 && r->u.i < 0 && res >= 0))
				/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
				errx(10, "overflow");
			l->u.i = res;
		} else {
			res = l->u.i - r->u.i;
			if ((l->u.i >= 0 && r->u.i < 0 && res <= 0) ||
			    (l->u.i < 0 && r->u.i > 0 && res >= 0))
				/* amiport: exit code 3 -> 10 (RETURN_ERROR) */
				errx(10, "overflow");
			l->u.i = res;
		}

		free_value(r);
	}

	return l;
}

/* Parse and evaluate comparison expressions */
struct val *
eval2(void)
{
	struct val     *l, *r;
	enum token	op;
	int64_t		v = 0, li, ri;

	l = eval3();
	while ((op = token) == EQ || op == NE || op == LT || op == GT ||
	    op == LE || op == GE) {
		nexttoken(0);
		r = eval3();

		if (is_integer(l, &li, NULL) && is_integer(r, &ri, NULL)) {
			switch (op) {
			case GT:
				v = (li >  ri);
				break;
			case GE:
				v = (li >= ri);
				break;
			case LT:
				v = (li <  ri);
				break;
			case LE:
				v = (li <= ri);
				break;
			case EQ:
				v = (li == ri);
				break;
			case NE:
				v = (li != ri);
				break;
			default:
				break;
			}
		} else {
			to_string(l);
			to_string(r);

			switch (op) {
			case GT:
				v = (strcoll(l->u.s, r->u.s) > 0);
				break;
			case GE:
				v = (strcoll(l->u.s, r->u.s) >= 0);
				break;
			case LT:
				v = (strcoll(l->u.s, r->u.s) < 0);
				break;
			case LE:
				v = (strcoll(l->u.s, r->u.s) <= 0);
				break;
			case EQ:
				v = (strcoll(l->u.s, r->u.s) == 0);
				break;
			case NE:
				v = (strcoll(l->u.s, r->u.s) != 0);
				break;
			default:
				break;
			}
		}

		free_value(l);
		free_value(r);
		l = make_int(v);
	}

	return l;
}

/* Parse and evaluate & expressions */
struct val *
eval1(void)
{
	struct val     *l, *r;

	l = eval2();
	while (token == AND) {
		nexttoken(0);
		r = eval2();

		if (is_zero_or_null(l) || is_zero_or_null(r)) {
			free_value(l);
			free_value(r);
			l = make_int(0);
		} else {
			free_value(r);
		}
	}

	return l;
}

/* Parse and evaluate | expressions */
struct val *
eval0(void)
{
	struct val     *l, *r;

	l = eval1();
	while (token == OR) {
		nexttoken(0);
		r = eval1();

		if (is_zero_or_null(l)) {
			free_value(l);
			l = r;
		} else {
			free_value(r);
		}
	}

	return l;
}

/* amiport: cleanup function for atexit -- frees argv expansion */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	struct val     *vp;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob.
	 * Although __nowild suppresses file globbing, we still call
	 * expand_argv to initialize __progname from argv[0]. */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge stubbed as macro -- no-op on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		/* amiport: exit code 2 -> 10 (RETURN_ERROR) */
		err(10, "pledge");

	if (argc > 1 && !strcmp(argv[1], "--"))
		argv++;

	av = argv + 1;

	nexttoken(0);
	vp = eval0();

	if (token != EOI)
		error();

	if (vp->type == integer)
		/* amiport: %lld for int64_t (long long) -- bebbo-gcc supports this */
		printf("%lld\n", vp->u.i);
	else
		printf("%s\n", vp->u.s);

	/* amiport: POSIX exit codes preserved -- expr returns 0=true, 1=false.
	 * These special POSIX semantics are intentional and must NOT be remapped
	 * to Amiga exit codes. AmigaOS scripts should use IF WARN (>=5) to test
	 * for false result if needed; exit(1) passes through. */
	return is_zero_or_null(vp);
}
