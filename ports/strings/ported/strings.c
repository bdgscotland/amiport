/*	$OpenBSD$	*/
/*
 * strings - find printable strings in files
 *
 * A minimal, self-contained implementation of the standard
 * Unix strings utility. Scans files (or stdin) for sequences
 * of printable characters of a given minimum length.
 *
 * Public domain.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> — activates exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: added <amiport/getopt.h> for getopt(), optarg, optind — maps to amiport_getopt() */
#include <amiport/getopt.h>
/* amiport: replaced <err.h> with <amiport/err.h> — bare <err.h> missing from bebbo-gcc libnix */
#include <amiport/err.h>
/* amiport: added <amiport/signal.h> for amiport_check_break() Ctrl-C polling */
#include <amiport/signal.h>
/* amiport: added <amiport/glob.h> for amiport_expand_argv() wildcard expansion */
#include <amiport/glob.h>

/* amiport: AmigaOS version string */
static const char *verstag = "$VER: strings 1.0 (11.04.2026)";

/* amiport: stack cookie — 8192 bytes sufficient for this utility */
long __stack = 8192;

#define DEFAULT_MIN 4

static int minlen = DEFAULT_MIN;
static int show_offset = 0;
static char offset_format = 'o'; /* d, o, or x */

/* amiport: cleanup function registered via atexit() to free expanded argv */
static void
cleanup(void)
{
    /* amiport: free wildcard-expanded argv on all exit paths including err()/errx() */
    amiport_free_argv();
    fflush(stdout);
}

static void
strings(FILE *fp, const char *fname)
{
	int ch;
	long offset = 0;
	int count = 0;
	static char buf[8192];
	/* amiport: perf-optimizer HIGH: fread into block buffer instead of fgetc per byte
	 * yields 3-5x speedup on 68000 by reducing JSR/stack frame overhead per byte */
	static unsigned char ibuf[8192];
	size_t nr, bi;

	while ((nr = fread(ibuf, 1, sizeof(ibuf), fp)) > 0) {
		/* amiport: Ctrl-C check once per block (not per byte) */
		if (amiport_check_break()) {
			fflush(stdout);
			return;
		}
		for (bi = 0; bi < nr; bi++) {
		ch = ibuf[bi];
		/* amiport: use explicit ASCII range check instead of isprint()
		 * because libnix isprint() treats 0x80-0xFF as printable */
		if ((ch >= 0x20 && ch <= 0x7E) || ch == '\t') {
			if (count < minlen) {
				if (count < (int)sizeof(buf) - 1)
					buf[count] = ch;
				count++;
				if (count == minlen) {
					buf[count] = '\0';
					if (show_offset) {
						switch (offset_format) {
						case 'd':
							printf("%7ld ", offset - count + 1);
							break;
						case 'x':
							printf("%7lx ", offset - count + 1);
							break;
						default:
							printf("%7lo ", offset - count + 1);
							break;
						}
					}
					fputs(buf, stdout);
				}
			} else {
				putchar(ch);
			}
		} else {
			if (count >= minlen)
				putchar('\n');
			count = 0;
		}
		offset++;
		} /* end for (bi) */
	} /* end while (fread) */
	if (count >= minlen)
		putchar('\n');
}

static void
usage(void)
{
	fprintf(stderr, "usage: strings [-a] [-n number] [-t format] [file ...]\n");
	/* amiport: exit(1) -> exit(10) — AmigaOS RETURN_ERROR; exit(1) is invisible to Amiga scripts */
	exit(10);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ch;
	int ret = 0;

	/* amiport: expand wildcards in argv (AmigaOS shell passes globs unexpanded) */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup so amiport_free_argv() runs on all exit paths */
	atexit(cleanup);

	/* amiport: getopt() provided by <amiport/unistd.h> via amiport_getopt() */
	while ((ch = getopt(argc, argv, "an:t:")) != -1) {
		switch (ch) {
		case 'a':
			/* scan whole file (default behavior) */
			break;
		case 'n':
			minlen = atoi(optarg);
			if (minlen < 1)
				/* amiport: errx(1, ...) -> errx(10, ...) — AmigaOS RETURN_ERROR */
				errx(10, "invalid minimum string length: %s", optarg);
			break;
		case 't':
			show_offset = 1;
			offset_format = optarg[0];
			if (offset_format != 'd' && offset_format != 'o' &&
			    offset_format != 'x')
				/* amiport: errx(1, ...) -> errx(10, ...) — AmigaOS RETURN_ERROR */
				errx(10, "invalid offset format: %c", offset_format);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		strings(stdin, "stdin");
	} else {
		for (; *argv; argv++) {
			if (strcmp(*argv, "-") == 0) {
				strings(stdin, "stdin");
			} else {
				fp = fopen(*argv, "rb");
				if (fp == NULL) {
					warn("%s", *argv);
					/* amiport: ret = 1 -> ret = 10 — AmigaOS RETURN_ERROR */
					ret = 10;
					continue;
				}
				strings(fp, *argv);
				/* amiport: guard fclose — never fclose(stdin), it crashes AmigaOS console handler */
				if (fp != stdin)
					fclose(fp);
			}
		}
	}

	return ret;
}
