/*	$OpenBSD: diff.c,v 1.69 2026/02/18 15:25:01 deraadt Exp $	*/

/*
 * Copyright (c) 2003 Todd C. Miller <millert@openbsd.org>
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
 *
 * Sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F39502-99-1-0512.
 */

#include <exec/types.h> /* amiport: Amiga exec types */
#include <dos/dos.h>    /* amiport: BPTR, FileInfoBlock */
#include <proto/dos.h>  /* amiport: Lock(), Examine(), UnLock() */

#include <amiport/sys/stat.h> /* amiport: replaced <sys/stat.h> */

#include <ctype.h>
#include <amiport/err.h> /* amiport: replaced <err.h> */
#include <errno.h>
#include <amiport/getopt.h> /* amiport: replaced <getopt.h> — getopt only, no getopt_long */
#include <amiport/stdlib.h> /* amiport: exit() override prevents libnix hang */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <amiport/unistd.h> /* amiport: replaced <unistd.h> */
#include <limits.h>

#include "diff.h"
#include "xmalloc.h"

/* amiport: __dead may not be defined on AmigaOS */
#ifndef __dead
#define __dead
#endif

/* amiport: Amiga version string */
static const char *verstag = "$VER: diff 1.95 (20.03.2026)";

/* amiport: Stack size cookie — diff can recurse deeply with -r */
LONG __stack = 65536;

int	 Nflag, Pflag, rflag, sflag, Tflag;
int	 diff_format, diff_context, status;
char	*start, *ifdefname, *diffargs, *label[2], *ignore_pats;
struct stat stb1, stb2;
struct excludes *excludes_list;
#ifdef HAS_REGEX /* amiport: regex is optional on AmigaOS */
regex_t	 ignore_re;
#endif

#define	OPTIONS	"0123456789abC:cdD:efhI:iL:nNPpqrS:sTtU:uwX:x:"
/* amiport: removed longopts[] table — getopt_long not available */

__dead void usage(void);
void push_excludes(char *);
void push_ignore_pats(char *);
void read_excludes_file(char *file);
void set_argstr(char **, char **);

/* amiport: vamos Lock() doesn't work on relative paths,
 * so we use fopen() to detect files vs directories.
 * For sizes, diffreg.c will use fseek/ftell after fopen. */
static int amiport_fill_stat(const char *path, struct amiport_stat *sb)
{
    BPTR lock;
    struct FileInfoBlock fib;
    FILE *fp;
    long size;

    memset(sb, 0, sizeof(*sb));

    /* Use Lock/Examine for all metadata — this is the authoritative path.
     * amiport: populates st_ino, st_dev, and st_mtime so same-file detection
     * and timestamp comparisons work correctly. */
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock) {
        memset(&fib, 0, sizeof(fib));
        if (Examine(lock, &fib)) {
            /* amiport: st_ino from fib_DiskKey — unique per file on volume */
            sb->st_ino = (unsigned long)fib.fib_DiskKey;
            /* amiport: st_dev — derive from volume node of the lock.
             * This distinguishes files on different volumes so same-file
             * detection (st_dev == st_dev && st_ino == st_ino) works
             * correctly across volumes. */
            {
                struct DevProc *dp = GetDeviceProc((CONST_STRPTR)path, NULL);
                if (dp) {
                    sb->st_dev = (unsigned long)dp->dvp_Port;
                    FreeDeviceProc(dp);
                } else {
                    sb->st_dev = 1; /* fallback */
                }
            }
            /* amiport: st_mtime — convert AmigaOS DateStamp to Unix time.
             * AmigaOS epoch is 1978-01-01; offset from Unix epoch is 252460800s */
            sb->st_mtime = (long)fib.fib_Date.ds_Days * 86400L
                         + (long)fib.fib_Date.ds_Minute * 60L
                         + (long)fib.fib_Date.ds_Tick / 50L
                         + 252460800L;
            if (fib.fib_DirEntryType > 0) {
                sb->st_mode = AMIPORT_S_IFDIR | 0755;
                sb->st_isdir = 1;
            } else {
                sb->st_mode = AMIPORT_S_IFREG | 0644;
                sb->st_size = fib.fib_Size;
            }
        }
        UnLock(lock);
        return 0;
    }

    /* Lock failed — last resort: try fopen() to detect a readable regular
     * file (e.g. on a filesystem where Lock() is unreliable under vamos).
     * st_ino/st_dev/st_mtime will be zero in this path. */
    fp = fopen(path, "rb");
    if (fp != NULL) {
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        fclose(fp);
        sb->st_mode = AMIPORT_S_IFREG | 0644;
        sb->st_size = size;
        sb->st_isdir = 0;
        return 0;
    }

    /* Nothing worked */
    errno = ENOENT;
    return -1;
}

int
main(int argc, char **argv)
{
	char *ep, **oargv;
	char *spliced0, *spliced1; /* amiport: track splice() allocations for free() */
	long  l;
	int   ch, dflags, lastch, gotstdin, prevoptind, newarg;

	oargv = argv;
	gotstdin = 0;
	dflags = 0;
	lastch = '\0';
	prevoptind = 1;
	newarg = 1;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) { /* amiport: replaced getopt_long() with getopt() */
		switch (ch) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (newarg)
				usage();	/* disallow -[0-9]+ */
			else if (lastch == 'c' || lastch == 'u')
				diff_context = 0;
			else if (!isdigit(lastch) || diff_context > INT_MAX / 10)
				usage();
			diff_context = (diff_context * 10) + (ch - '0');
			break;
		case 'a':
			dflags |= D_FORCEASCII;
			break;
		case 'b':
			dflags |= D_FOLDBLANKS;
			break;
		case 'C':
		case 'c':
			diff_format = D_CONTEXT;
			if (optarg != NULL) {
				l = strtol(optarg, &ep, 10);
				if (*ep != '\0' || l < 0 || l >= INT_MAX)
					usage();
				diff_context = (int)l;
			} else
				diff_context = 3;
			break;
		case 'd':
			dflags |= D_MINIMAL;
			break;
		case 'D':
			diff_format = D_IFDEF;
			ifdefname = optarg;
			break;
		case 'e':
			diff_format = D_EDIT;
			break;
		case 'f':
			diff_format = D_REVERSE;
			break;
		case 'h':
			/* silently ignore for backwards compatibility */
			break;
		case 'I':
#ifdef HAS_REGEX /* amiport: -I requires regex support */
			push_ignore_pats(optarg);
#else
			warnx("-I option requires regex support (not available)"); /* amiport: stub */
#endif
			break;
		case 'i':
			dflags |= D_IGNORECASE;
			break;
		case 'L':
			if (label[0] == NULL)
				label[0] = optarg;
			else if (label[1] == NULL)
				label[1] = optarg;
			else
				usage();
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'n':
			diff_format = D_NREVERSE;
			break;
		case 'p':
			dflags |= D_PROTOTYPE;
			break;
		case 'P':
			Pflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'q':
			diff_format = D_BRIEF;
			break;
		case 'S':
			start = optarg;
			break;
		case 's':
			sflag = 1;
			break;
		case 'T':
			Tflag = 1;
			break;
		case 't':
			dflags |= D_EXPANDTABS;
			break;
		case 'U':
		case 'u':
			diff_format = D_UNIFIED;
			if (optarg != NULL) {
				l = strtol(optarg, &ep, 10);
				if (*ep != '\0' || l < 0 || l >= INT_MAX)
					usage();
				diff_context = (int)l;
			} else
				diff_context = 3;
			break;
		case 'w':
			dflags |= D_IGNOREBLANKS;
			break;
		case 'X':
			read_excludes_file(optarg);
			break;
		case 'x':
			push_excludes(optarg);
			break;
		default:
			usage();
			break;
		}
		lastch = ch;
		newarg = optind != prevoptind;
		prevoptind = optind;
	}
	argc -= optind;
	argv += optind;

#ifndef __AMIGA__ /* amiport: removed pledge/unveil — OpenBSD-specific sandboxing */
	if (unveil("/tmp", "rwc") == -1)
		err(10, "unveil /tmp");
	if (unveil("/", "r") == -1)
		err(10, "unveil /");
	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		err(10, "pledge");
#endif

	/*
	 * Do sanity checks, fill in stb1 and stb2 and call the appropriate
	 * driver routine.  Both drivers use the contents of stb1 and stb2.
	 */
	if (argc != 2)
		usage();
#ifdef HAS_REGEX /* amiport: regex compilation is optional */
	if (ignore_pats != NULL) {
		char buf[BUFSIZ];
		int error;

		if ((error = regcomp(&ignore_re, ignore_pats,
				     REG_NEWLINE | REG_EXTENDED)) != 0) {
			regerror(error, &ignore_re, buf, sizeof(buf));
			if (*ignore_pats != '\0')
				errx(10, "%s: %s", ignore_pats, buf);
			else
				errx(10, "%s", buf);
		}
	}
#endif
	if (strcmp(argv[0], "-") == 0) {
		/* amiport: fstat(STDIN_FILENO) replaced — treat stdin as regular file */
		memset(&stb1, 0, sizeof(stb1));
		stb1.st_mode = AMIPORT_S_IFREG | 0644;
		gotstdin = 1;
	} else if (amiport_fill_stat(argv[0], &stb1) != 0)
		err(10, "%s", argv[0]);
	if (strcmp(argv[1], "-") == 0) {
		/* amiport: fstat(STDIN_FILENO) replaced — treat stdin as regular file */
		memset(&stb2, 0, sizeof(stb2));
		stb2.st_mode = AMIPORT_S_IFREG | 0644;
		gotstdin = 1;
	} else if (amiport_fill_stat(argv[1], &stb2) != 0)
		err(10, "%s", argv[1]);
	if (gotstdin && (S_ISDIR(stb1.st_mode) || S_ISDIR(stb2.st_mode)))
		errx(10, "can't compare - to a directory");
	set_argstr(oargv, argv);
	if (S_ISDIR(stb1.st_mode) && S_ISDIR(stb2.st_mode)) {
		if (diff_format == D_IFDEF)
			errx(10, "-D option not supported with directories");
		diffdir(argv[0], argv[1], dflags);
	} else {
		spliced0 = NULL;
		spliced1 = NULL;
		if (S_ISDIR(stb1.st_mode)) {
			/* amiport: save splice() result for free() — splice() allocates */
			spliced0 = splice(argv[0], argv[1]);
			argv[0] = spliced0;
			if (amiport_fill_stat(argv[0], &stb1) == -1) /* amiport: replaced stat() */
				err(10, "%s", argv[0]); /* amiport: exit(2) -> exit(10) RETURN_ERROR */
		}
		if (S_ISDIR(stb2.st_mode)) {
			/* amiport: save splice() result for free() — splice() allocates */
			spliced1 = splice(argv[1], argv[0]);
			argv[1] = spliced1;
			if (amiport_fill_stat(argv[1], &stb2) == -1) /* amiport: replaced stat() */
				err(10, "%s", argv[1]); /* amiport: exit(2) -> exit(10) RETURN_ERROR */
		}
		print_status(diffreg(argv[0], argv[1], dflags), argv[0], argv[1],
		    "");
		/* amiport: free splice()-allocated strings before exit */
		if (spliced0 != NULL)
			free(spliced0);
		if (spliced1 != NULL)
			free(spliced1);
	}
	/* amiport: remap POSIX exit codes to Amiga exit codes.
	 * status==0: RETURN_OK, status==1: files differ -> RETURN_WARN(5),
	 * status>=2: error -> RETURN_ERROR(10). */
	if (status == 0)
		exit(0);
	else if (status == 1)
		exit(5);
	else
		exit(10);
}

void
set_argstr(char **av, char **ave)
{
	size_t argsize;
	char **ap;

	/* amiport: replaced *ave - *av pointer arithmetic with explicit length
	 * calculation — vamos does not guarantee sequential argv string layout. */
	argsize = 4 + 1; /* "diff" + NUL */
	for (ap = av + 1; ap < ave; ap++) {
		if (strcmp(*ap, "--") != 0)
			argsize += 1 + strlen(*ap); /* " " + arg */
	}
	diffargs = xmalloc(argsize);
	strlcpy(diffargs, "diff", argsize);
	for (ap = av + 1; ap < ave; ap++) {
		if (strcmp(*ap, "--") != 0) {
			strlcat(diffargs, " ", argsize);
			strlcat(diffargs, *ap, argsize);
		}
	}
}

/*
 * Read in an excludes file and push each line.
 */
void
read_excludes_file(char *file)
{
	FILE *fp;
	char *pattern;
	char buf[8192]; /* amiport: replaced fgetln() with fgets() */
	size_t len;

	if (strcmp(file, "-") == 0)
		fp = stdin;
	else if ((fp = fopen(file, "r")) == NULL)
		err(10, "%s", file);
	while (fgets(buf, sizeof(buf), fp) != NULL) { /* amiport: replaced fgetln() with fgets() */
		len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n')
			len--;
		pattern = xmalloc(len + 1);
		memcpy(pattern, buf, len);
		pattern[len] = '\0';
		push_excludes(pattern);
	}
	if (strcmp(file, "-") != 0)
		fclose(fp);
}

/*
 * Push a pattern onto the excludes list.
 */
void
push_excludes(char *pattern)
{
	struct excludes *entry;

	entry = xmalloc(sizeof(*entry));
	entry->pattern = pattern;
	entry->next = excludes_list;
	excludes_list = entry;
}

void
push_ignore_pats(char *pattern)
{
	size_t len;

	if (ignore_pats == NULL)
		ignore_pats = xstrdup(pattern);
	else {
		/* old + "|" + new + NUL */
		len = strlen(ignore_pats) + strlen(pattern) + 2;
		ignore_pats = xreallocarray(ignore_pats, 1, len);
		strlcat(ignore_pats, "|", len);
		strlcat(ignore_pats, pattern, len);
	}
}

void
print_only(const char *path, size_t dirlen, const char *entry)
{
	if (dirlen > 1)
		dirlen--;
	printf("Only in %.*s: %s\n", (int)dirlen, path, entry);
}

void
print_status(int val, char *path1, char *path2, char *entry)
{
	switch (val) {
	case D_BINARY:
		printf("Binary files %s%s and %s%s differ\n",
		    path1, entry, path2, entry);
		break;
	case D_DIFFER:
		if (diff_format == D_BRIEF)
			printf("Files %s%s and %s%s differ\n",
			    path1, entry, path2, entry);
		break;
	case D_SAME:
		if (sflag)
			printf("Files %s%s and %s%s are identical\n",
			    path1, entry, path2, entry);
		break;
	case D_MISMATCH1:
		printf("File %s%s is a directory while file %s%s is a regular file\n",
		    path1, entry, path2, entry);
		break;
	case D_MISMATCH2:
		printf("File %s%s is a regular file while file %s%s is a directory\n",
		    path1, entry, path2, entry);
		break;
	case D_SKIPPED1:
		printf("File %s%s is not a regular file or directory and was skipped\n",
		    path1, entry);
		break;
	case D_SKIPPED2:
		printf("File %s%s is not a regular file or directory and was skipped\n",
		    path2, entry);
		break;
	}
}

__dead void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: diff [-abdipTtw] [-c | -e | -f | -n | -q | -u] [-I pattern] [-L label]\n"
	    "            file1 file2\n"
	    "       diff [-abdipTtw] [-I pattern] [-L label] -C number file1 file2\n"
	    "       diff [-abditw] [-I pattern] -D string file1 file2\n"
	    "       diff [-abdipTtw] [-I pattern] [-L label] -U number file1 file2\n"
	    "       diff [-abdiNPprsTtw] [-c | -e | -f | -n | -q | -u] [-I pattern]\n"
	    "            [-L label] [-S name] [-X file] [-x pattern] dir1 dir2\n");

	exit(10); /* amiport: exit(2) -> exit(10) RETURN_ERROR */
}
