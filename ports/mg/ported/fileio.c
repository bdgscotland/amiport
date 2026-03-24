/*	$OpenBSD: fileio.c,v 1.111 2023/03/30 19:00:02 op Exp $	*/

/* This file is in the public domain. */

/*
 *	POSIX fileio.c
 */

/* amiport: replaced POSIX headers with AmigaOS shims */
#ifdef __AMIGA__
#include <amiport/sys/stat.h>  /* amiport: stat/fstat shim */
#include <amiport/unistd.h>    /* amiport: open/read/write/close/getcwd etc. */
#include <amiport/stdio.h>     /* amiport: fopen/fclose/tmpfile/mkstemp macros */
#include <amiport/dirent.h>    /* amiport: opendir/readdir/closedir */
#include <amiport/stdlib.h>    /* amiport: exit → amiport_exit, getenv → amiport_getenv */
#include <amiport/stdio_ext.h> /* amiport: asprintf/vasprintf */
/* amiport: no sys/wait.h, sys/resource.h, pwd.h on AmigaOS */
/* amiport: stub getpwuid/getpwnam via HOME env var (see expandtilde) */
struct passwd { char *pw_dir; char *pw_name; };
/* amiport: stub popen/pclose — gzip not supported on AmigaOS */
# define popen(cmd, mode) (NULL)
# define pclose(fp)       (0)
/* amiport: umask() stub — AmigaOS has no umask */
# ifndef _UMASK_STUB
static unsigned int _amiport_umask_val = 0;
static inline unsigned int umask(unsigned int m) { unsigned int old = _amiport_umask_val; _amiport_umask_val = m; return old; }
#  define _UMASK_STUB 1
# endif
/* amiport: fchmod/fchown/futimens → no-ops on AmigaOS */
# define fchmod(fd, mode) (0)   /* amiport: no file permissions on AmigaOS */
# define fchown(fd, u, g) (0)   /* amiport: no file ownership on AmigaOS */
# define futimens(fd, ts) (0)   /* amiport: no futimens on AmigaOS */
#else
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#endif
#include <errno.h>
#ifndef __AMIGA__
#include <fcntl.h>   /* amiport: open()/O_RDONLY etc. only used in non-Amiga code paths */
#include <signal.h>  /* amiport: no signals on AmigaOS */
#include <unistd.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "def.h"
#include "kbd.h"
#include "pathnames.h"

#ifndef MAXNAMLEN
#define MAXNAMLEN 255
#endif

#ifndef DEFFILEMODE
#define DEFFILEMODE 0666
#endif

#ifndef GUNZIP
#define GUNZIP "gunzip -c"
#endif

static char *bkuplocation(const char *);
static int   bkupleavetmp(const char *);
static int   isgzip(const char *);

static char *bkupdir;
static int   leavetmp = 0;	/* 1 = leave any '~' files in tmp dir */

/*
 * Open a file for reading.
 */
int
ffropen(FILE **ffp, const char *fn, struct buffer *bp)
{
	if (isgzip(fn)) {
#ifndef __AMIGA__
		/* amiport: popen() and gzip not available on AmigaOS — isgzip() always returns 0 */
		char cmd[strlen(fn) + sizeof(GUNZIP) + 2];

                snprintf(cmd, sizeof(cmd), "%s %s", GUNZIP, fn);
                if ((*ffp = popen(cmd, "r")) == NULL)
			goto filerr;

		ffstat(*ffp, bp);
		if (bp)
			bp->b_flag = BFIGNDIRTY;

		return (FIOGZIP);
#endif
	}

	if ((*ffp = fopen(fn, "r")) == NULL) {
	filerr:
		if (errno == ENOENT)
			return (FIOFNF);
		return (FIOERR);
	}

	/* If 'fn' is a directory open it with dired. */
	if (fisdir(fn) == TRUE)
		return (FIODIR);

	ffstat(*ffp, bp);

	return (FIOSUC);
}

/*
 * Update stat/dirty info
 */
void
ffstat(FILE *ffp, struct buffer *bp)
{
#ifdef __AMIGA__
	/* amiport: AmigaOS struct stat has no st_uid/st_gid/st_mtim.
	 * Set fi_mode to indicate file exists; zero-init timestamps. */
	if (bp) {
		bp->b_fi.fi_mode = 0x8000; /* amiport: mark as valid */
		bp->b_fi.fi_uid = 0;
		bp->b_fi.fi_gid = 0;
		bp->b_fi.fi_mtime.tv_sec = 0;
		bp->b_fi.fi_mtime.tv_nsec = 0;
		bp->b_flag &= ~(BFIGNDIRTY | BFDIRTY);
	}
#else
	struct stat	sb;

	if (bp && fstat(fileno(ffp), &sb) == 0) {
		/* set highorder bit to make sure this isn't all zero */
		bp->b_fi.fi_mode = sb.st_mode | 0x8000;
		bp->b_fi.fi_uid = sb.st_uid;
		bp->b_fi.fi_gid = sb.st_gid;
		bp->b_fi.fi_mtime = sb.st_mtim;
		/* Clear the ignore flag */
		bp->b_flag &= ~(BFIGNDIRTY | BFDIRTY);
	}
#endif
}

/*
 * Update the status/dirty info. If there is an error,
 * there's not a lot we can do.
 */
int
fupdstat(struct buffer *bp)
{
	FILE *ffp;

	if ((ffp = fopen(bp->b_fname, "r")) == NULL) {
		if (errno == ENOENT)
			return (FIOFNF);
		return (FIOERR);
	}
	ffstat(ffp, bp);
	(void)ffclose(ffp, bp);
	return (FIOSUC);
}

/*
 * Open a file for writing.
 */
int
ffwopen(FILE ** ffp, const char *fn, struct buffer *bp)
{
#ifdef __AMIGA__
	/* amiport: use fopen("w") directly — cannot mix amiport_open+fdopen (crash-patterns #12).
	 * AmigaOS has no file permissions — fchmod/fchown/umask are no-ops. */
	if ((*ffp = fopen(fn, "w")) == NULL) {
		ffp = NULL;
		dobeep();
		ewprintf("Cannot open file for writing : %s", strerror(errno));
		return (FIOERR);
	}
	/* amiport: fchmod/fchown/futimens are stubbed as no-ops above */
	return (FIOSUC);
#else
	int	fd;
	mode_t	fmode = DEFFILEMODE;

	if (bp && bp->b_fi.fi_mode)
		fmode = bp->b_fi.fi_mode & 07777;

	fd = open(fn, O_RDWR | O_CREAT | O_TRUNC, fmode);
	if (fd == -1) {
		ffp = NULL;
		dobeep();
		ewprintf("Cannot open file for writing : %s", strerror(errno));
		return (FIOERR);
	}

	if ((*ffp = fdopen(fd, "w")) == NULL) {
		dobeep();
		ewprintf("Cannot open file for writing : %s", strerror(errno));
		close(fd);
		return (FIOERR);
	}

	/*
	 * If we have file information, use it.  We don't bother to check for
	 * errors, because there's no a lot we can do about it.  Certainly
	 * trying to change ownership will fail if we aren't root.  That's
	 * probably OK.  If we don't have info, no need to get it, since any
	 * future writes will do the same thing.
	 */
	if (bp && bp->b_fi.fi_mode) {
		fchmod(fd, bp->b_fi.fi_mode & 07777);
		if (fchown(fd, bp->b_fi.fi_uid, bp->b_fi.fi_gid) && errno != EPERM)
			ewprintf("Cannot set owner : %s", strerror(errno));
	}
	return (FIOSUC);
#endif
}

/*
 * Close a file.
 */
int
ffclose(FILE *ffp, struct buffer *bp)
{
	if (fclose(ffp) == 0)
		return (FIOSUC);
	return (FIOERR);
}

/*
 * Write a buffer to the already opened file. bp points to the
 * buffer. Return the status.
 */
int
ffputbuf(FILE *ffp, struct buffer *bp, int eobnl)
{
	struct line	*lp, *lpend;

	lpend = bp->b_headp;

	for (lp = lforw(lpend); lp != lpend; lp = lforw(lp)) {
		if ((int)fwrite(ltext(lp), 1, llength(lp), ffp) != llength(lp)) {
			dobeep();
			ewprintf("Write I/O error");
			return (FIOERR);
		}
		if (lforw(lp) != lpend)		/* no implied \n on last line */
			putc(*bp->b_nlchr, ffp);
	}
	if (eobnl) {
		lnewline_at(lback(lpend), llength(lback(lpend)));
		putc(*bp->b_nlchr, ffp);
	}
	return (FIOSUC);
}

/*
 * Read a line from a file, and store the bytes
 * in the supplied buffer. Stop on end of file or end of
 * line.  When FIOEOF is returned, there is a valid line
 * of data without the normally implied \n.
 * If the line length exceeds nbuf, FIOLONG is returned.
 */
int
ffgetline(FILE *ffp, char *buf, int nbuf, int *nbytes)
{
	int	c, i;

	i = 0;
	while ((c = getc(ffp)) != EOF && c != *curbp->b_nlchr) {
		buf[i++] = c;
		if (i >= nbuf)
			return (FIOLONG);
	}
	if (c == EOF && ferror(ffp) != FALSE) {
		dobeep();
		ewprintf("File read error");
		return (FIOERR);
	}
	*nbytes = i;
	return (c == EOF ? FIOEOF : FIOSUC);
}

/*
 * Make a backup copy of "fname".  On Unix the backup has the same
 * name as the original file, with a "~" on the end; this seems to
 * be newest of the new-speak. The error handling is all in "file.c".
 * We do a copy instead of a rename since otherwise another process
 * with an open fd will get the backup, not the new file.  This is
 * a problem when using mg with things like crontab and vipw.
 */
int
fbackupfile(const char *fn)
{
#ifdef __AMIGA__
	/* amiport: simplified backup using fopen() — no open()/mkstemp()/fchmod() on AmigaOS.
	 * AmigaOS has exclusive write locks (crash-patterns #19) — cannot have two
	 * handles open simultaneously. Use simple rename-style backup: copy fn → fn~. */
	char		*nname;
	char		*bkpth;
	FILE		*from, *to;
	char		 buf[BUFSIZ];
	size_t		 nread;
	int		 ok = TRUE;

	if ((bkpth = bkuplocation(fn)) == NULL)
		return (FALSE);

	if (amiport_asprintf(&nname, "%s~", bkpth) == -1) {
		dobeep();
		ewprintf("Can't allocate backup file name");
		free(bkpth);
		return (ABORT);
	}
	free(bkpth);

	if ((from = fopen(fn, "rb")) == NULL) {
		free(nname);
		return (FALSE);
	}
	/* amiport: AmigaDOS exclusive lock — must close source before opening dest
	 * if they share a path, but here src=fn, dst=fn~ so they differ — OK */
	if ((to = fopen(nname, "wb")) == NULL) {
		fclose(from);
		free(nname);
		return (FALSE);
	}
	while ((nread = fread(buf, 1, sizeof(buf), from)) > 0) {
		if (fwrite(buf, 1, nread, to) != nread) {
			ok = FALSE;
			break;
		}
	}
	if (ferror(from))
		ok = FALSE;
	fclose(from);
	fclose(to);
	if (!ok) {
		amiport_unlink(nname);
		ewprintf("Backup write error");
	}
	free(nname);
	return (ok ? TRUE : FALSE);
#else
	struct stat	 sb;
	struct timespec	 new_times[2];
	int		 from, to, serrno;
	ssize_t		 nread;
	mode_t		 omask;
	char		 buf[BUFSIZ];
	char		*nname, *tname, *bkpth;

	if (stat(fn, &sb) == -1) {
		dobeep();
		ewprintf("Can't stat %s : %s", fn, strerror(errno));
		return (FALSE);
	}

	if ((bkpth = bkuplocation(fn)) == NULL)
		return (FALSE);

	if (asprintf(&nname, "%s~", bkpth) == -1) {
		dobeep();
		ewprintf("Can't allocate backup file name : %s", strerror(errno));
		free(bkpth);
		return (ABORT);
	}
	if (asprintf(&tname, "%s.XXXXXXXXXX", bkpth) == -1) {
		dobeep();
		ewprintf("Can't allocate temp file name : %s", strerror(errno));
		free(bkpth);
		free(nname);
		return (ABORT);
	}
	free(bkpth);

	if ((from = open(fn, O_RDONLY)) == -1) {
		free(nname);
		free(tname);
		return (FALSE);
	}

	omask = umask(0600);
	to = mkstemp(tname);
	umask(omask);
	if (to == -1) {
		serrno = errno;
		close(from);
		free(nname);
		free(tname);
		errno = serrno;
		return (FALSE);
	}
	while ((nread = read(from, buf, sizeof(buf))) > 0) {
		if (write(to, buf, (size_t)nread) != nread) {
			nread = -1;
			break;
		}
	}
	serrno = errno;
	(void) fchmod(to, (sb.st_mode & 0777));

	new_times[0] = sb.st_atim;
	new_times[1] = sb.st_mtim;
	futimens(to, new_times);

	close(from);
	close(to);
	if (nread == -1) {
		if (unlink(tname) == -1)
			ewprintf("Can't unlink temp : %s", strerror(errno));
	} else {
		if (rename(tname, nname) == -1) {
			ewprintf("Can't rename temp : %s", strerror(errno));
			(void) unlink(tname);
			nread = -1;
		}
	}
	free(nname);
	free(tname);
	errno = serrno;

	return (nread == -1 ? FALSE : TRUE);
#endif
}

/*
 * Convert "fn" to a canonicalized absolute filename, replacing
 * a leading ~/ with the user's home dir, following symlinks, and
 * remove all occurrences of /./ and /../
 */
char *
adjustname(const char *fn, int slashslash)
{
	static char	 fnb[PATH_MAX];
	const char	*cp, *ep = NULL;
	char		*path;

	if (slashslash == TRUE) {
		cp = fn + strlen(fn) - 1;
		for (; cp >= fn; cp--) {
			if (ep && (*cp == '/')) {
				fn = ep;
				break;
			}
			if (*cp == '/' || *cp == '~')
				ep = cp;
			else
				ep = NULL;
		}
	}
	if ((path = expandtilde(fn)) == NULL)
		return (NULL);

#ifdef __AMIGA__
	/* amiport: realpath() not available on AmigaOS — use path as-is */
	(void)strlcpy(fnb, path, sizeof(fnb));
#else
	if (realpath(path, fnb) == NULL)
		(void)strlcpy(fnb, path, sizeof(fnb));
#endif

	free(path);
	return (fnb);
}

/*
 * Find a startup file for the user and return its name. As a service
 * to other pieces of code that may want to find a startup file (like
 * the terminal driver in particular), accepts a suffix to be appended
 * to the startup file name.
 */
FILE *
startupfile(char *suffix, char *conffile, char *path, size_t len)
{
	FILE		*ffp = NULL;
	char		*home;
	int		 ret;

	home = getenv("HOME"); /* amiport: returns malloc'd string — must free */
	if (home == NULL || *home == '\0') {
#ifdef __AMIGA__
		if (home) free(home);
#endif
		goto nohome;
	}

	if (conffile != NULL) {
		(void)strlcpy(path, conffile, len);
	} else if (suffix == NULL) {
#ifdef __AMIGA__
		/* amiport: _PATH_MG_STARTUP is "S:.mg" on AmigaOS — no %s prefix */
		ret = snprintf(path, len, "%s", _PATH_MG_STARTUP);
#else
		ret = snprintf(path, len, _PATH_MG_STARTUP, home);
#endif
		if (ret < 0 || ret >= (int)len) {
#ifdef __AMIGA__
			free(home);
#endif
			return (NULL);
		}
	} else {
#ifdef __AMIGA__
		/* amiport: _PATH_MG_TERM is "S:.mg-%s" — only suffix needed */
		ret = snprintf(path, len, _PATH_MG_TERM, suffix);
#else
		ret = snprintf(path, len, _PATH_MG_TERM, home, suffix);
#endif
		if (ret < 0 || ret >= (int)len) {
#ifdef __AMIGA__
			free(home);
#endif
			return (NULL);
		}
	}
#ifdef __AMIGA__
	free(home); /* amiport: amiport_getenv returns malloc'd string — free after use */
	home = NULL;
#endif

	ret = ffropen(&ffp, path, NULL);
	if (ret == FIOSUC)
		return (ffp);
	if (ffp) {
		if (ret == FIOGZIP)
			(void)pclose(ffp);
		else
			(void)ffclose(ffp, NULL);
		ffp = NULL;
	}
nohome:
#ifdef STARTUPFILE
	if (suffix == NULL) {
		ret = snprintf(path, len, "%s", STARTUPFILE);
		if (ret < 0 || ret >= (int)len)
			return (NULL);
	} else {
		ret = snprintf(path, len, "%s-%s", STARTUPFILE,
		    suffix);
		if (ret < 0 || ret >= (int)len)
			return (NULL);
	}

	ret = ffropen(&ffp, path, NULL);
	if (ret == FIOSUC)
		return (ffp);
	if (ffp) {
		if (ret == FIOGZIP)
			(void)pclose(ffp);
		else
			(void)ffclose(ffp, NULL);
	}
#endif
	return (NULL);
}

int
copy(char *frname, char *toname)
{
#ifdef __AMIGA__
	/* amiport: use fopen() — cannot mix amiport_open+fdopen (crash-patterns #12).
	 * fchmod/fchown are no-ops on AmigaOS. */
	FILE	*ifd, *ofd;
	char	 buf[BUFSIZ];
	size_t	 sr;

	if ((ifd = fopen(frname, "rb")) == NULL)
		return (FALSE);
	if ((ofd = fopen(toname, "wb")) == NULL) {
		fclose(ifd);
		return (FALSE);
	}
	while ((sr = fread(buf, 1, sizeof(buf), ifd)) > 0) {
		if (fwrite(buf, 1, sr, ofd) != sr) {
			ewprintf("write error : %s", strerror(errno));
			break;
		}
	}
	if (ferror(ifd))
		ewprintf("Read error : %s", strerror(errno));
	fclose(ifd);
	fclose(ofd);
	return (TRUE);
#else
	int	ifd, ofd;
	char	buf[BUFSIZ];
	mode_t	fmode = DEFFILEMODE;	/* XXX?? */
	struct	stat orig;
	ssize_t	sr;

	if ((ifd = open(frname, O_RDONLY)) == -1)
		return (FALSE);
	if (fstat(ifd, &orig) == -1) {
		dobeep();
		ewprintf("fstat: %s", strerror(errno));
		close(ifd);
		return (FALSE);
	}

	if ((ofd = open(toname, O_WRONLY|O_CREAT|O_TRUNC, fmode)) == -1) {
		close(ifd);
		return (FALSE);
	}
	while ((sr = read(ifd, buf, sizeof(buf))) > 0) {
		if (write(ofd, buf, (size_t)sr) != sr) {
			ewprintf("write error : %s", strerror(errno));
			break;
		}
	}
	if (fchmod(ofd, orig.st_mode) == -1)
		ewprintf("Cannot set original mode : %s", strerror(errno));

	if (sr == -1) {
		ewprintf("Read error : %s", strerror(errno));
		close(ifd);
		close(ofd);
		return (FALSE);
	}
	if (fchown(ofd, orig.st_uid, orig.st_gid) && errno != EPERM)
		ewprintf("Cannot set owner : %s", strerror(errno));

	(void) close(ifd);
	(void) close(ofd);

	return (TRUE);
#endif
}

/*
 * return list of file names that match the name in buf.
 */
struct list *
make_file_list(char *buf)
{
	char		*dir, *file, *cp;
	size_t		 len, preflen;
	int		 ret;
	DIR		*dirp;
	struct dirent	*dent;
	struct list	*last, *current;
	char		 fl_name[NFILEN + 2];
	char		 prefixx[NFILEN + 1];

	/*
	 * We need three different strings:

	 * dir - the name of the directory containing what the user typed.
	 *  Must be a real unix file name, e.g. no ~user, etc..
	 *  Must not end in /.
	 * prefix - the portion of what the user typed that is before the
	 *  names we are going to find in the directory.  Must have a
	 * trailing / if the user typed it.
	 * names from the directory - We open dir, and return prefix
	 * concatenated with names.
	 */

	/* first we get a directory name we can look up */
	/*
	 * Names ending in . are potentially odd, because adjustname will
	 * treat foo/bar/.. as a foo/, whereas we are
	 * interested in names starting with ..
	 */
	len = strlen(buf);
	if (len && buf[len - 1] == '.') {
		buf[len - 1] = 'x';
		dir = adjustname(buf, TRUE);
		buf[len - 1] = '.';
	} else
		dir = adjustname(buf, TRUE);
	if (dir == NULL)
		return (NULL);
	/*
	 * If the user typed a trailing / or the empty string
	 * he wants us to use his file spec as a directory name.
	 */
	if (len && buf[len - 1] != '/') {
		file = strrchr(dir, '/');
		if (file) {
			*file = '\0';
			if (*dir == '\0')
				dir = "/";
		} else
			return (NULL);
	}
	/* Now we get the prefix of the name the user typed. */
	if (strlcpy(prefixx, buf, sizeof(prefixx)) >= sizeof(prefixx))
		return (NULL);
	cp = strrchr(prefixx, '/');
	if (cp == NULL)
		prefixx[0] = '\0';
	else
		cp[1] = '\0';

	preflen = strlen(prefixx);
	/* cp is the tail of buf that really needs to be compared. */
	cp = buf + preflen;
	len = strlen(cp);

	/*
	 * Now make sure that file names will fit in the buffers allocated.
	 * SV files are fairly short.  For BSD, something more general would
	 * be required.
	 */
	if (preflen > NFILEN - MAXNAMLEN)
		return (NULL);

	/* loop over the specified directory, making up the list of files */

	/*
	 * Note that it is worth our time to filter out names that don't
	 * match, even though our caller is going to do so again, and to
	 * avoid doing the stat if completion is being done, because stat'ing
	 * every file in the directory is relatively expensive.
	 */

	dirp = opendir(dir);
	if (dirp == NULL)
		return (NULL);
	last = NULL;

	while ((dent = readdir(dirp)) != NULL) {
		char		statname[NFILEN + 2];
		struct stat	statbuf;
		int		isdir = 0;

#ifdef __AMIGA__
		if (strncasecmp(cp, dent->d_name, len) != 0) /* amiport: AmigaOS is case-insensitive */
#else
		if (strncmp(cp, dent->d_name, len) != 0)
#endif
			continue;

		statbuf.st_mode = 0;
		ret = snprintf(statname, sizeof(statname), "%s/%s",
		    dir, dent->d_name);
		if (ret < 0 || ret > (int)sizeof(statname) - 1)
			continue;
		if (stat(statname, &statbuf) < 0)
			continue;
		if (S_ISDIR(statbuf.st_mode))
			isdir = 1;

		if ((current = malloc(sizeof(struct list))) == NULL) {
			free_file_list(last);
			closedir(dirp);
			return (NULL);
		}
		ret = snprintf(fl_name, sizeof(fl_name),
		    "%s%s%s", prefixx, dent->d_name, isdir ? "/" : "");
		if (ret < 0 || ret >= (int)sizeof(fl_name)) {
			free(current);
			continue;
		}
		current->l_next = last;
		current->l_name = strdup(fl_name);
		last = current;
	}
	closedir(dirp);

	return (last);
}

#ifndef fisdir
/*
 * Test if a supplied filename refers to a directory
 * Returns ABORT on error, TRUE if directory. FALSE otherwise
 */
int
fisdir(const char *fname)
{
	struct stat	statbuf;

	if (stat(fname, &statbuf) != 0)
		return (ABORT);

	if (S_ISDIR(statbuf.st_mode))
		return (TRUE);

	return (FALSE);
}
#endif

/*
 * Check the mtime of the supplied filename.
 * Return TRUE if last mtime matches, FALSE if not,
 * If the stat fails, return TRUE and try the save anyway
 */
int
fchecktime(struct buffer *bp)
{
	struct stat sb;

	if (stat(bp->b_fname, &sb) == -1)
		return (TRUE);

#ifdef __AMIGA__
	/* amiport: AmigaOS struct stat has no st_mtim — always report unchanged */
	(void)sb;
	return (TRUE);
#else
	if (bp->b_fi.fi_mtime.tv_sec != sb.st_mtim.tv_sec ||
	    bp->b_fi.fi_mtime.tv_nsec != sb.st_mtim.tv_nsec)
		return (FALSE);
#endif

	return (TRUE);

}

/*
 * Check if ends with .gz
 */
static int
isgzip(const char *fn)
{
#ifdef __AMIGA__
	/* amiport: popen() not available on AmigaOS — gzip files not supported */
	(void)fn;
	return 0;
#else
        size_t len;

        len = strlen(fn);
        if (len > 4 && !strcmp(&fn[len - 3], ".gz")) {
                return 1;
        }

        return 0;
#endif
}

/*
 * Location of backup file. This function creates the correct path.
 */
static char *
bkuplocation(const char *fn)
{
	struct stat sb;
	char *ret;

	if (bkupdir != NULL && (stat(bkupdir, &sb) == 0) &&
	    S_ISDIR(sb.st_mode) && !bkupleavetmp(fn)) {
		char fname[NFILEN];
		const char *c;
		int i = 0, len;

		c = fn;
		len = strlen(bkupdir);

		while (*c != '\0') {
			/* Make sure we don't go over combined:
		 	* strlen(bkupdir + '/' + fname + '\0')
		 	*/
			if (i >= NFILEN - len - 1)
				return (NULL);
			if (*c == '/') {
				fname[i] = '!';
			} else if (*c == '!') {
				if (i >= NFILEN - len - 2)
					return (NULL);
				fname[i++] = '!';
				fname[i] = '!';
			} else
				fname[i] = *c;
			i++;
			c++;
		}
		fname[i] = '\0';
		if (amiport_asprintf(&ret, "%s/%s", bkupdir, fname) == -1) /* amiport: replaced asprintf() */
			return (NULL);

	} else if ((ret = strndup(fn, NFILEN)) == NULL)
		return (NULL);

	return (ret);
}

int
backuptohomedir(int f, int n)
{
	const char	*c = _PATH_MG_DIR;
	char		*p;

	if (bkupdir == NULL) {
		p = adjustname(c, TRUE);
		if (p == NULL)
			return (FALSE);

		bkupdir = strndup(p, NFILEN);
		if (bkupdir == NULL)
			return(FALSE);

		if (amiport_mkdir(bkupdir, 0700) == -1 && errno != EEXIST) { /* amiport: replaced mkdir() */
			free(bkupdir);
			bkupdir = NULL;
		}
	} else {
		free(bkupdir);
		bkupdir = NULL;
	}

	return (TRUE);
}

/*
 * For applications that use mg as the editor and have a desire to keep
 * '~' files in /tmp, toggle the location: /tmp | ~/.mg.d
 */
int
toggleleavetmp(int f, int n)
{
	leavetmp = !leavetmp;

	return (TRUE);
}

/*
 * Returns TRUE if fn is located in the temp directory and we want to save
 * those backups there.
 */
int
bkupleavetmp(const char *fn)
{
	if (!leavetmp)
		return(FALSE);

#ifdef __AMIGA__
	if (strncmp(fn, "T:", 2) == 0 || strncmp(fn, "RAM:T/", 6) == 0)
		return (TRUE); /* amiport: /tmp → T: on AmigaOS */
#else
	if (strncmp(fn, "/tmp", 4) == 0)
		return (TRUE);
#endif

	return (FALSE);
}

/*
 * Expand file names beginning with '~' if appropriate:
 *   1, if ./~fn exists, continue without expanding tilde.
 *   2, else, if username 'fn' exists, expand tilde with home directory path.
 *   3, otherwise, continue and create new buffer called ~fn.
 */
char *
expandtilde(const char *fn)
{
	struct passwd	*pw;
	struct stat	 statbuf;
	const char	*cp;
	char		 user[LOGIN_NAME_MAX], path[NFILEN];
	char		*ret;
	size_t		 ulen, plen;

	path[0] = '\0';

	if (fn[0] != '~' || stat(fn, &statbuf) == 0) {
		if ((ret = strndup(fn, NFILEN)) == NULL)
			return (NULL);
		return(ret);
	}
	cp = strchr(fn, '/');
	if (cp == NULL)
		cp = fn + strlen(fn); /* point to the NUL byte */
	ulen = cp - &fn[1];
	if (ulen >= sizeof(user)) {
		if ((ret = strndup(fn, NFILEN)) == NULL)
			return (NULL);
		return(ret);
	}
#ifdef __AMIGA__
	/* amiport: getpwuid()/getpwnam() not available — use HOME env var.
	 * Only ~/ expansion supported; ~user/ expansion falls through. */
	{
		static struct passwd amiga_pw;
		static char amiga_home[NFILEN];
		char *htmp;
		if (ulen == 0) {
			htmp = getenv("HOME"); /* amiport: malloc'd — must free */
			if (htmp != NULL) {
				strlcpy(amiga_home, htmp, sizeof(amiga_home));
				free(htmp);
				amiga_pw.pw_dir = amiga_home;
				pw = &amiga_pw;
			} else {
				pw = NULL;
			}
		} else {
			pw = NULL; /* amiport: ~user/ not supported on AmigaOS */
		}
	}
#else
	if (ulen == 0) /* ~/ or ~ */
		pw = getpwuid(geteuid());
	else { /* ~user/ or ~user */
		memcpy(user, &fn[1], ulen);
		user[ulen] = '\0';
		pw = getpwnam(user);
	}
#endif
	if (pw != NULL) {
		plen = strlcpy(path, pw->pw_dir, sizeof(path));
		if (plen == 0 || path[plen - 1] != '/') {
			if (strlcat(path, "/", sizeof(path)) >= sizeof(path)) {
				dobeep();				
				ewprintf("Path too long");
				return (NULL);
			}
		}
		fn = cp;
		if (*fn == '/')
			fn++;
	}
	if (strlcat(path, fn, sizeof(path)) >= sizeof(path)) {
		dobeep();
		ewprintf("Path too long");
		return (NULL);
	}
	if ((ret = strndup(path, NFILEN)) == NULL)
		return (NULL);

	return (ret);
}
