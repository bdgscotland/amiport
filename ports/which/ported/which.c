/*	$OpenBSD: which.c,v 1.27 2019/01/25 00:19:27 millert Exp $	*/

/*
 * Copyright (c) 1997 Todd C. Miller <millert@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h> */
#include <amiport/sys/stat.h>
/* amiport: removed <sys/sysctl.h> -- not used on AmigaOS */

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
#include <errno.h>
#include <limits.h>
/* amiport: removed <paths.h> -- not available on AmigaOS; using local path defines */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() -> amiport_exit() */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> -- provides amiport_access, amiport_getenv */
#include <amiport/unistd.h>
/* amiport: added <amiport/glob.h> for amiport_expand_argv / amiport_free_argv */
#include <amiport/glob.h>
/* amiport: added <amiport/getopt.h> -- use amiport getopt; provides getopt/optind/optarg macros */
#include <amiport/getopt.h>
/* amiport: added <amiport/pwd.h> for setuid/geteuid stubs */
#include <amiport/pwd.h>
/* amiport: added <amiport/grp.h> for setgid/getegid stubs */
#include <amiport/grp.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: which 1.27 (26.03.2026)";

/* amiport: Stack size cookie -- 16384 bytes is sufficient for this program */
LONG __stack = 16384;

/*
 * amiport: PATH constants for AmigaOS.
 * AmigaOS uses the C: assign as the standard location for commands.
 * Multiple directories can be listed with PATH as colon-separated on AmigaOS
 * (same as POSIX), but the default fallback is C:.
 */
#define _PATH_STDPATH "C:"
#define _PATH_DEFPATH "C:"

/* amiport: stub pledge/unveil -- not available on AmigaOS */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

#define PROG_WHICH	1
#define PROG_WHEREIS	2

extern char *__progname;

int findprog(char *, char *, int, int);
/* amiport: removed __dead attribute from usage() -- OpenBSD-specific, not available on AmigaOS */
static void usage(void);

/*
 * which(1) -- find an executable(s) in the user's path
 * whereis(1) -- find an executable(s) in the default user path
 *
 * Return values:
 *	0 - all executables found
 *	1 - some found, some not
 *	2 - none found
 */

/* amiport: atexit cleanup -- frees expanded argv and flushes stdout */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	char *path;
	char *path_alloc; /* amiport: track malloc'd PATH from amiport_getenv for free() */
	size_t n;
	int ch, allmatches = 0, notfound = 0, progmode = PROG_WHICH;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	while ((ch = getopt(argc, argv, "a")) != -1)
		switch (ch) {
		case 'a':
			allmatches = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	path_alloc = NULL;
	if (strcmp(__progname, "whereis") == 0) {
		progmode = PROG_WHEREIS;
		path = _PATH_STDPATH;
	} else {
		/* amiport: replaced getenv() with amiport_getenv() -- returns malloc'd string */
		path_alloc = amiport_getenv("PATH");
		if (path_alloc == NULL || *path_alloc == '\0') {
			/* amiport: free empty PATH result before using fallback */
			if (path_alloc != NULL) {
				free(path_alloc);
				path_alloc = NULL;
			}
			path = _PATH_DEFPATH;
		} else {
			path = path_alloc;
		}
	}

	/* amiport: setgid/setuid are no-ops on AmigaOS (single-user, no real permissions).
	 * The calls succeed (return 0) via shim. err() left in place for correctness. */
	if (setgid(getegid()))
		err(10, "Can't set gid to %u", getegid()); /* amiport: RETURN_ERROR */
	if (setuid(geteuid()))
		err(10, "Can't set uid to %u", geteuid()); /* amiport: RETURN_ERROR */

	if (pledge("stdio rpath", NULL) == -1)
		err(20, "pledge"); /* amiport: RETURN_FAIL -- but pledge() is stubbed to 0 */

	for (n = 0; n < (size_t)argc; n++)
		if (findprog(argv[n], path, progmode, allmatches) == 0)
			notfound++;

	/* amiport: free malloc'd PATH before exit */
	if (path_alloc != NULL)
		free(path_alloc);

	return ((notfound == 0) ? 0 : ((notfound == (size_t)argc) ? 20 : 5));
	/* amiport: exit codes remapped: 0=RETURN_OK, 5=RETURN_WARN (some found),
	 * 20=RETURN_FAIL (none found). POSIX 1/2 -> Amiga 5/20. */
}

int
findprog(char *prog, char *path, int progmode, int allmatches)
{
	char *p, filename[PATH_MAX];
	int len, rval = 0;
	struct amiport_stat sbuf; /* amiport: using amiport_stat struct directly */
	char *pathcpy;

	/* Special case if prog contains '/' */
	if (strchr(prog, '/')) {
		/* amiport: stat() and access() mapped via <amiport/sys/stat.h> and
		 * <amiport/unistd.h> macros to amiport_stat() and amiport_access() */
		if ((stat(prog, &sbuf) == 0) && S_ISREG(sbuf.st_mode) &&
		    access(prog, X_OK) == 0) {
			(void)puts(prog);
			return (1);
		} else {
			warnx("%s: Command not found.", prog);
			return (0);
		}
	}

	if ((path = strdup(path)) == NULL)
		err(10, "strdup"); /* amiport: RETURN_ERROR */
	pathcpy = path;

	while ((p = strsep(&pathcpy, ":")) != NULL) {
		if (*p == '\0')
			p = ".";

		len = strlen(p);
		while (len > 0 && p[len-1] == '/')
			p[--len] = '\0';	/* strip trailing '/' */

		len = snprintf(filename, sizeof(filename), "%s/%s", p, prog);
		if (len < 0 || len >= (int)sizeof(filename)) {
			warnc(ENAMETOOLONG, "%s/%s", p, prog);
			free(path);
			return (0);
		}
		/* amiport: stat() and access() mapped via shim macros */
		if ((stat(filename, &sbuf) == 0) && S_ISREG(sbuf.st_mode) &&
		    access(filename, X_OK) == 0) {
			(void)puts(filename);
			rval = 1;
			if (!allmatches) {
				free(path);
				return (rval);
			}
		}
	}
	(void)free(path);

	/* whereis(1) is silent on failure. */
	if (!rval && progmode != PROG_WHEREIS)
		warnx("%s: Command not found.", prog);
	return (rval);
}

/* amiport: removed __dead attribute -- OpenBSD-specific, not available on AmigaOS */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-a] name ...\n", __progname);
	exit(10); /* amiport: RETURN_ERROR */
}
