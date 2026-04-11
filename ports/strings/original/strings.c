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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#define DEFAULT_MIN 4

static int minlen = DEFAULT_MIN;
static int show_offset = 0;
static char offset_format = 'o'; /* d, o, or x */

static void
strings(FILE *fp, const char *fname)
{
	int ch;
	long offset = 0;
	int count = 0;
	static char buf[8192];

	while ((ch = fgetc(fp)) != EOF) {
		if (isprint(ch) || ch == '\t') {
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
	}
	if (count >= minlen)
		putchar('\n');
}

static void
usage(void)
{
	fprintf(stderr, "usage: strings [-a] [-n number] [-t format] [file ...]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ch;
	int ret = 0;

	while ((ch = getopt(argc, argv, "an:t:")) != -1) {
		switch (ch) {
		case 'a':
			/* scan whole file (default behavior) */
			break;
		case 'n':
			minlen = atoi(optarg);
			if (minlen < 1)
				errx(1, "invalid minimum string length: %s", optarg);
			break;
		case 't':
			show_offset = 1;
			offset_format = optarg[0];
			if (offset_format != 'd' && offset_format != 'o' &&
			    offset_format != 'x')
				errx(1, "invalid offset format: %c", offset_format);
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
					ret = 1;
					continue;
				}
				strings(fp, *argv);
				if (fp != stdin)
					fclose(fp);
			}
		}
	}

	return ret;
}
