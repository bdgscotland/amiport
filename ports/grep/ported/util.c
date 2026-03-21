/*	$OpenBSD: util.c,v 1.68 2023/11/15 00:50:43 millert Exp $	*/
/* amiport: Ported to AmigaOS 3.x */

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

/* amiport: replaced system includes with grep.h (which includes compat.h) */
/* amiport: removed <fts.h> — using fts replacement from compat.h */
/* amiport: removed <stdbool.h> — bool defined in compat.h */
/* amiport: removed <zlib.h> — no gzip support (NOZ) */
#include "grep.h"

/*
 * Process a file line by line...
 */

static int	linesqueued;
static int	procline(str_t *l, int);
static int	grep_search(fastgrep_t *, char *, size_t, regmatch_t *pmatch, int);
static bool	grep_cmp(const char *, const char *, size_t);
static void	grep_revstr(unsigned char *, int);

int
grep_tree(char **argv)
{
	FTS	*fts;
	FTSENT	*p;
	int	c, fts_flags;
	char	*dot_argv[] = { ".", NULL };
	char	*path;

	/* default to . if no path given */
	if (argv[0] == NULL)
		argv = dot_argv;

	c = 0;

	fts_flags = FTS_PHYSICAL | FTS_NOSTAT | FTS_NOCHDIR;

	/* amiport: fts_open/fts_read/fts_close from compat.h fts replacement */
	if (!(fts = fts_open(argv, fts_flags, NULL)))
		err(AMIGA_RETURN_ERROR, NULL);
	while ((p = fts_read(fts)) != NULL) {
		switch (p->fts_info) {
		case FTS_DNR:
			break;
		case FTS_ERR:
			file_err = 1;
			if(!sflag)
				warnc(p->fts_errno, "%s", p->fts_path);
			break;
		case FTS_D:
		case FTS_DP:
			break;
		default:
			path = p->fts_path;
			/* skip "./" if implied */
			if (argv == dot_argv && p->fts_pathlen >= 2)
				path += 2;
			c |= procfile(path);
			break;
		}
	}
	if (errno)
		err(AMIGA_RETURN_ERROR, "fts_read");
	fts_close(fts);
	return c;
}

int
procfile(char *fn)
{
	str_t ln;
	file_t *f;
	int t, z, nottext, overflow = 0;
	unsigned long long c;

	mcount = mlimit;

	if (fn == NULL) {
		fn = "(standard input)";
		f = grep_fdopen(STDIN_FILENO);
	} else {
		f = grep_open(fn);
	}
	if (f == NULL) {
		if (errno == EISDIR)
			return 0;
		file_err = 1;
		if (!sflag)
			warn("%s", fn);
		return 0;
	}

	nottext = grep_bin_file(f);
	if (nottext && binbehave == BIN_FILE_SKIP) {
		grep_close(f);
		return 0;
	}

	ln.file = fn;
	if (labelname)
		ln.file = (char *)labelname;
	ln.line_no = 0;
	ln.len = 0;
	linesqueued = 0;
	tail = 0;
	ln.off = -1;

	if (Bflag > 0)
		initqueue();
	for (c = 0;  c == 0 || !(lflag || qflag); ) {
		if (mflag && mlimit == 0)
			break;
		ln.off += ln.len + 1;
		if ((ln.dat = grep_fgetln(f, &ln.len)) == NULL)
			break;
		if (ln.len > 0 && ln.dat[ln.len - 1] == '\n')
			--ln.len;
		ln.line_no++;

		z = tail;

		if ((t = procline(&ln, nottext)) == 0 && Bflag > 0 && z == 0) {
			enqueue(&ln);
			linesqueued++;
		}
		if (ULLONG_MAX - c < (unsigned long long)t)
			overflow = 1;
		else
			c += t;
		if (mflag && mcount <= 0 && tail <= 0)
			break;
	}
	if (Bflag > 0)
		clearqueue();
	grep_close(f);

	if (cflag) {
		if (!hflag)
			printf("%s%c", ln.file, nullflag ? '\0' : ':');
		/* amiport: use %lu for count — libnix may not support %llu */
		printf("%lu%s\n", (unsigned long)c, overflow ? "+" : "");
	}
	if (lflag && c != 0)
		printf("%s%c", fn, nullflag ? '\0' : '\n');
	if (Lflag && c == 0)
		printf("%s%c", fn, nullflag ? '\0' : '\n');
	if (c && !cflag && !lflag && !Lflag &&
	    binbehave == BIN_FILE_BIN && nottext && !qflag)
		printf("Binary file %s matches\n", fn);

	return overflow || c != 0;
}


/*
 * Process an individual line in a file. Return non-zero if it matches.
 */

#define isword(x) (isalnum((unsigned char)x) || (x) == '_')

static int
procline(str_t *l, int nottext)
{
	regmatch_t	pmatch;
	int		c, i, r, counted;
	regoff_t	offset;
	char		saved;

	pmatch.rm_so = 0;
	pmatch.rm_eo = 0;

	/* size_t will be converted to regoff_t. ssize_t is guaranteed to fit
	 * into regoff_t */
	if (l->len > SSIZE_MAX) {
		errx(AMIGA_RETURN_ERROR, "Line is too big to process");
	}

	/* amiport: NUL-terminate the line for our Tier 2 regex emulation.
	 * The original code uses REG_STARTEND to bound the match region,
	 * but our regex emu needs NUL-terminated strings. */
	saved = l->dat[l->len];
	l->dat[l->len] = '\0';

	c = 0;
	i = 0;
	counted = 0;
	if (matchall) {
		c = 1;
		goto print;
	}
	if (mflag && mcount <= 0)
		goto print;

	for (i = 0; i < patterns; i++) {
		offset = 0;
redo:
		if (fg_pattern[i].pattern) {
			int flags = 0;
			if (offset)
				flags |= REG_NOTBOL;
			r = grep_search(&fg_pattern[i], l->dat + offset,
			    l->len - offset, &pmatch, flags);
			pmatch.rm_so += offset;
			pmatch.rm_eo += offset;
		} else {
			int flags = eflags;
			if (offset)
				flags |= REG_NOTBOL;
			/* amiport: instead of REG_STARTEND, pass substring
			 * to regexec (our emu needs NUL-terminated strings) */
			pmatch.rm_so = 0;
			pmatch.rm_eo = l->len - offset;
			r = regexec(&r_pattern[i], l->dat + offset, 1, &pmatch, flags);
			pmatch.rm_so += offset;
			pmatch.rm_eo += offset;
		}
		if (r == 0 && xflag) {
			if (pmatch.rm_so != 0 || pmatch.rm_eo != (int)l->len)
				r = REG_NOMATCH;
		}
		if (r == 0) {
			c = 1;
			if (oflag && pmatch.rm_so != pmatch.rm_eo)
				goto print;
			break;
		}
	}
	if (oflag) {
		/* amiport: restore saved char */
		l->dat[l->len] = saved;
		return c;
	}
print:
	if (vflag)
		c = !c;

	/* Count the matches if there is a match limit (but only once). */
	if (mflag && !counted) {
		mcount -= c;
		counted = 1;
	}

	if (c && binbehave == BIN_FILE_BIN && nottext) {
		/* amiport: restore saved char */
		l->dat[l->len] = saved;
		return c; /* Binary file */
	}

	if ((tail > 0 || c) && !cflag && !qflag) {
		if (c) {
			if (first > 0 && tail == 0 && (Aflag || (Bflag &&
			    Bflag < linesqueued)))
				printf("--\n");
			first = 1;
			tail = Aflag;
			if (Bflag > 0)
				printqueue();
			linesqueued = 0;
			printline(l, ':', oflag ? &pmatch : NULL);
		} else {
			printline(l, '-', oflag ? &pmatch : NULL);
			tail--;
		}
	}
	if (oflag && !matchall) {
		offset = pmatch.rm_eo;
		goto redo;
	}

	/* amiport: restore saved char */
	l->dat[l->len] = saved;
	return c;
}

void
fgrepcomp(fastgrep_t *fg, const unsigned char *pat)
{
	int i;

	/* Initialize. */
	fg->patternLen = strlen((const char *)pat);
	fg->bol = 0;
	fg->eol = 0;
	fg->wmatch = wflag;
	fg->reversedSearch = 0;

	/*
	 * Make a copy and upper case it for later if in -i mode,
	 * else just copy the pointer.
	 */
	if (iflag) {
		fg->pattern = grep_malloc(fg->patternLen + 1);
		for (i = 0; i < fg->patternLen; i++)
			fg->pattern[i] = toupper(pat[i]);
		fg->pattern[fg->patternLen] = '\0';
	} else
		fg->pattern = (unsigned char *)pat;	/* really const */

	/* Preprocess pattern. */
	for (i = 0; i <= UCHAR_MAX; i++)
		fg->qsBc[i] = fg->patternLen;
	for (i = 1; i < fg->patternLen; i++) {
		fg->qsBc[fg->pattern[i]] = fg->patternLen - i;
		/*
		 * If case is ignored, make the jump apply to both upper and
		 * lower cased characters.  As the pattern is stored in upper
		 * case, apply the same to the lower case equivalents.
		 */
		if (iflag)
			fg->qsBc[tolower(fg->pattern[i])] = fg->patternLen - i;
	}
}

/*
 * Returns: -1 on failure, 0 on success
 */
int
fastcomp(fastgrep_t *fg, const char *pat)
{
	int i;
	int bol = 0;
	int eol = 0;
	int shiftPatternLen;
	int hasDot = 0;
	int firstHalfDot = -1;
	int firstLastHalfDot = -1;
	int lastHalfDot = 0;

	/* Initialize. */
	fg->patternLen = strlen(pat);
	fg->bol = 0;
	fg->eol = 0;
	fg->wmatch = 0;
	fg->reversedSearch = 0;

	/* Remove end-of-line character ('$'). */
	if (fg->patternLen > 0 && pat[fg->patternLen - 1] == '$') {
		eol++;
		fg->eol = 1;
		fg->patternLen--;
	}

	/* Remove beginning-of-line character ('^'). */
	if (pat[0] == '^') {
		bol++;
		fg->bol = 1;
		fg->patternLen--;
	}

	/* Remove enclosing [[:<:]] and [[:>:]] (word match). */
	if (wflag) {
		/* basic re's use \( \), extended re's ( ) */
		int extra = Eflag ? 1 : 2;
		fg->patternLen -= 14 + 2 * extra;
		fg->wmatch = 7 + extra;
	} else if (fg->patternLen >= 14 &&
	    strncmp(pat + fg->bol, "[[:<:]]", 7) == 0 &&
	    strncmp(pat + fg->bol + fg->patternLen - 7, "[[:>:]]", 7) == 0) {
		fg->patternLen -= 14;
		fg->wmatch = 7;
	}

	/*
	 * Copy pattern minus '^' and '$' characters as well as word
	 * match character classes at the beginning and ending of the
	 * string respectively.
	 */
	fg->pattern = grep_malloc(fg->patternLen + 1);
	memcpy(fg->pattern, pat + bol + fg->wmatch, fg->patternLen);
	fg->pattern[fg->patternLen] = '\0';

	/* Look for ways to cheat...er...avoid the full regex engine. */
	for (i = 0; i < fg->patternLen; i++)
	{
		switch (fg->pattern[i]) {
		case '.':
			hasDot = i;
			if (i < fg->patternLen / 2) {
				if (firstHalfDot < 0)
					/* Closest dot to the beginning */
					firstHalfDot = i;
			} else {
				/* Closest dot to the end of the pattern. */
				lastHalfDot = i;
				if (firstLastHalfDot < 0)
					firstLastHalfDot = i;
			}
			break;
		case '(': case ')':
		case '{': case '}':
			/* Special in BRE if preceded by '\\' */
		case '?':
		case '+':
		case '|':
			/* Not special in BRE. */
			if (!Eflag)
				goto nonspecial;
			/* fall through */
		case '\\':
		case '*':
		case '[': case ']':
			/* Free memory and let others know this is empty. */
			free(fg->pattern);
			fg->pattern = NULL;
			return (-1);
		default:
nonspecial:
			if (iflag)
				fg->pattern[i] = toupper(fg->pattern[i]);
			break;
		}
	}

	/*
	 * Determine if a reverse search would be faster based on the placement
	 * of the dots.
	 */
	if ((!(lflag || cflag || oflag)) && ((!(bol || eol)) &&
	    ((lastHalfDot) && ((firstHalfDot < 0) ||
	    ((fg->patternLen - (lastHalfDot + 1)) < firstHalfDot))))) {
		fg->reversedSearch = 1;
		hasDot = fg->patternLen - (firstHalfDot < 0 ?
		    firstLastHalfDot : firstHalfDot) - 1;
		grep_revstr(fg->pattern, fg->patternLen);
	}

	/*
	 * Normal Quick Search would require a shift based on the position the
	 * next character after the comparison is within the pattern.  With
	 * wildcards, the position of the last dot effects the maximum shift
	 * distance.
	 */

	/* Adjust the shift based on location of the last dot ('.'). */
	shiftPatternLen = fg->patternLen - hasDot;

	/* Preprocess pattern. */
	for (i = 0; i <= UCHAR_MAX; i++)
		fg->qsBc[i] = shiftPatternLen;
	for (i = hasDot + 1; i < fg->patternLen; i++) {
		fg->qsBc[fg->pattern[i]] = fg->patternLen - i;
		if (iflag)
			fg->qsBc[tolower(fg->pattern[i])] = fg->patternLen - i;
	}

	/*
	 * Put pattern back to normal after pre-processing to allow for easy
	 * comparisons later.
	 */
	if (fg->reversedSearch)
		grep_revstr(fg->pattern, fg->patternLen);

	return (0);
}

/*
 * Word boundaries using regular expressions are defined as the point
 * of transition from a non-word char to a word char, or vice versa.
 */
#define wmatch(d, l, s, e)	\
	((s == 0 || !isword(d[s-1])) && (e == l || !isword(d[e])) && \
	  e > s && isword(d[s]) && isword(d[e-1]))

static int
grep_search(fastgrep_t *fg, char *data, size_t dataLen, regmatch_t *pmatch,
    int flags)
{
	regoff_t j;
	int rtrnVal = REG_NOMATCH;

	pmatch->rm_so = -1;
	pmatch->rm_eo = -1;

	/* No point in going farther if we do not have enough data. */
	if (dataLen < (size_t)fg->patternLen)
		return (rtrnVal);

	/* Only try once at the beginning or ending of the line. */
	if (fg->bol || fg->eol) {
		if (fg->bol && (flags & REG_NOTBOL))
			return 0;
		/* Simple text comparison. */
		if (dataLen >= (size_t)fg->patternLen) {
			if (fg->eol)
				j = dataLen - fg->patternLen;
			else
				j = 0;
			if (!((fg->bol && fg->eol) && (dataLen != (size_t)fg->patternLen)))
				if (grep_cmp((const char *)fg->pattern, data + j,
				    fg->patternLen)) {
					pmatch->rm_so = j;
					pmatch->rm_eo = j + fg->patternLen;
					if (!fg->wmatch || wmatch(data, (int)dataLen,
					    pmatch->rm_so, pmatch->rm_eo))
						rtrnVal = 0;
				}
		}
	} else if (fg->reversedSearch) {
		/* Quick Search algorithm. */
		j = dataLen;
		do {
			if (grep_cmp((const char *)fg->pattern,
			    data + j - fg->patternLen,
			    fg->patternLen)) {
				pmatch->rm_so = j - fg->patternLen;
				pmatch->rm_eo = j;
				if (!fg->wmatch || wmatch(data, (int)dataLen,
				    pmatch->rm_so, pmatch->rm_eo)) {
					rtrnVal = 0;
					break;
				}
			}
			if (j == (regoff_t)fg->patternLen)
				break;
			j -= fg->qsBc[(unsigned char)data[j - fg->patternLen - 1]];
		} while (j >= (regoff_t)fg->patternLen);
	} else {
		/* Quick Search algorithm. */
		j = 0;
		do {
			if (grep_cmp((const char *)fg->pattern, data + j,
			    fg->patternLen)) {
				pmatch->rm_so = j;
				pmatch->rm_eo = j + fg->patternLen;
				if (fg->patternLen == 0 || !fg->wmatch ||
				    wmatch(data, (int)dataLen, pmatch->rm_so,
				    pmatch->rm_eo)) {
					rtrnVal = 0;
					break;
				}
			}

			if (j + fg->patternLen == (int)dataLen)
				break;
			else
				j += fg->qsBc[(unsigned char)data[j + fg->patternLen]];
		} while (j <= (int)(dataLen - fg->patternLen));
	}

	return (rtrnVal);
}


void *
grep_malloc(size_t size)
{
	void	*ptr;

	if ((ptr = malloc(size)) == NULL)
		err(AMIGA_RETURN_ERROR, "malloc");
	return ptr;
}

void *
grep_calloc(size_t nmemb, size_t size)
{
	void	*ptr;

	if ((ptr = calloc(nmemb, size)) == NULL)
		err(AMIGA_RETURN_ERROR, "calloc");
	return ptr;
}

void *
grep_realloc(void *ptr, size_t size)
{
	if ((ptr = realloc(ptr, size)) == NULL)
		err(AMIGA_RETURN_ERROR, "realloc");
	return ptr;
}

void *
grep_reallocarray(void *ptr, size_t nmemb, size_t size)
{
	if ((ptr = reallocarray(ptr, nmemb, size)) == NULL)
		err(AMIGA_RETURN_ERROR, "reallocarray");
	return ptr;
}

/*
 * Returns:	true on success, false on failure
 *
 * amiport perf: three code paths ordered by invocation frequency.
 *   1. Fflag, !iflag (fixed-string, case-sensitive) — use memcmp.
 *      Avoids per-character branches and lets libnix use longword compares.
 *      On 68000, memcmp saves ~3-4 cycles/char vs the branch-heavy loop.
 *      For a 20-char pattern on a 1000-line file this is ~80K cycles/file.
 *   2. !iflag (regex with dot-wildcard, case-sensitive) — dot check only.
 *   3. iflag (case-insensitive) — full fold comparison.  Pattern is stored
 *      uppercase by fgrepcomp()/fastcomp(), so compare against toupper(data).
 */
static bool
grep_cmp(const char *pat, const char *data, size_t len)
{
	size_t i;

	/* Fast path: fixed-string, case-sensitive (fgrep -F without -i). */
	if (Fflag && !iflag)
		return (memcmp(pat, data, len) == 0);

	/* Regex, case-sensitive: only special char is '.'. */
	if (!iflag) {
		for (i = 0; i < len; i++) {
			if (pat[i] != data[i] && pat[i] != '.')
				return false;
		}
		return true;
	}

	/* Case-insensitive: pat stored as uppercase; dot wildcard applies when
	 * not in Fflag mode. */
	for (i = 0; i < len; i++) {
		if (pat[i] == data[i])
			continue;
		if (pat[i] == toupper((unsigned char)data[i]))
			continue;
		if (!Fflag && pat[i] == '.')
			continue;
		return false;
	}
	return true;
}

static void
grep_revstr(unsigned char *str, int len)
{
	int i;
	char c;

	for (i = 0; i < len / 2; i++) {
		c = str[i];
		str[i] = str[len - i - 1];
		str[len - i - 1] = c;
	}
}

void
printline(str_t *line, int sep, regmatch_t *pmatch)
{
	int n;

	n = 0;
	if (!hflag) {
		fputs(line->file, stdout);
		if (nullflag)
			putchar(0);
		else
			++n;
	}
	if (nflag) {
		if (n)
			putchar(sep);
		/* amiport: use %ld — libnix may not support %lld */
		printf("%ld", (long)line->line_no);
		++n;
	}
	if (bflag) {
		if (n)
			putchar(sep);
		printf("%ld", (long)(line->off +
		    (pmatch ? pmatch->rm_so : 0)));
		++n;
	}
	if (n)
		putchar(sep);
	if (pmatch)
		fwrite(line->dat + pmatch->rm_so,
		    pmatch->rm_eo - pmatch->rm_so, 1, stdout);
	else
		fwrite(line->dat, line->len, 1, stdout);
	putchar('\n');
}
