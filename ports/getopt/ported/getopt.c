/*	$OpenBSD: getopt.c,v 1.10 2015/10/09 01:37:07 deraadt Exp $	*/

/*
 * This material, written by Henry Spencer, was released by him
 * into the public domain and is thus not subject to any copyright.
 */

/* amiport: AmigaOS 3.x port */
/* amiport: Amiga version string */
static const char *verstag = "$VER: getopt 1.10 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

/* amiport: suppress wildcard expansion -- argv[1] is an option spec string,
 * not a filename pattern; expanding it would corrupt the option list */
int __nowild = 1;

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

#include <stdio.h>
#include <amiport/stdlib.h>  /* amiport: replaced <stdlib.h> -- activates exit() -> amiport_exit() */
#include <amiport/unistd.h>  /* amiport: replaced <unistd.h> */
#include <amiport/getopt.h>  /* amiport: use amiport getopt (libnix getopt_long is broken, crash-patterns #17) */
#include <amiport/err.h>
#include <amiport/glob.h>    /* amiport: provides amiport_expand_argv/amiport_free_argv/__progname */

static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	extern int optind;
	extern char *optarg;
	int c;
	int status = 0;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: exit code 1 -> 10 (RETURN_ERROR) */

	optind = 2;	/* Past the program name and the option letters. */
	while ((c = getopt(argc, argv, argv[1])) != -1)
		switch (c) {
		case '?':
			status = 10;	/* amiport: exit code 1 -> 10 (RETURN_ERROR) -- getopt routine gave message */
			break;
		default:
			if (optarg != NULL)
				printf(" -%c %s", c, optarg);
			else
				printf(" -%c", c);
			break;
		}
	printf(" --");
	for (; optind < argc; optind++)
		printf(" %s", argv[optind]);
	printf("\n");
	exit(status);
}
