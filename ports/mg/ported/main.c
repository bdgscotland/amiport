/*	$OpenBSD: main.c,v 1.94 2023/03/30 19:01:25 op Exp $	*/

/* This file is in the public domain. */

/*
 *	Mainline.
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: mg 3.7 (24.03.2026)";

/* amiport: Stack size cookie — 65536 for recursive text editor */
long __stack = 65536;

/* amiport: suppress wildcard expansion — mg takes filenames, not patterns */
/* Note: mg does process filenames so expansion IS wanted, but since users
 * may pass literal filenames, __nowild is not set — amiport_expand_argv not used */

#ifdef __AMIGA__
#include <amiport/err.h>       /* amiport: err/errx/warn/warnx */
#include <amiport/stdlib.h>    /* amiport: exit → amiport_exit, getenv → amiport_getenv */
#include <amiport/unistd.h>    /* amiport: POSIX unistd shim */
#include <amiport/getopt.h>    /* amiport: getopt/optarg/optind — libnix getopt_long is broken */
#include <amiport/termios.h>   /* amiport: termios shim */
#else
#include <err.h>
#endif
#include <limits.h>
/* amiport: setlocale(LC_CTYPE,"") → setlocale(LC_ALL,"C") on AmigaOS */
#include <locale.h>
#ifndef __AMIGA__
#include <signal.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __AMIGA__
#include <termios.h>
#include <unistd.h>
#endif

#include "def.h"
#include "kbd.h"
#include "funmap.h"
#include "macro.h"

#ifdef  MGLOG
#include "log.h"
#endif

int		 thisflag;			/* flags, this command	*/
int		 lastflag;			/* flags, last command	*/
int		 curgoal;			/* goal column		*/
int		 startrow;			/* row to start		*/
int		 doaudiblebell;			/* audible bell toggle	*/
int		 dovisiblebell;			/* visible bell toggle	*/
int		 dblspace;			/* sentence end #spaces	*/
int		 allbro;			/* all buffs read-only	*/
int		 batch;				/* for regress tests	*/
struct buffer	*curbp;				/* current buffer	*/
struct buffer	*bheadp;			/* BUFFER list head	*/
struct mgwin	*curwp;				/* current window	*/
struct mgwin	*wheadp;			/* MGWIN listhead	*/
struct vhead	 varhead;			/* Variable list head	*/
char		 pat[NPAT];			/* pattern		*/

static void	 edinit(struct buffer *);
static void	 pty_init(void);
extern void	 closetags(void);

static __dead void
usage(int code)
{
	fprintf(stderr, "usage: %s [-hnR] [-b file] [-f mode] [-u file] "
	    "[+number] [file ...]\n",
	    PACKAGE_NAME);
	exit(code);
}

int
main(int argc, char **argv)
{
	FILE		*ffp;
	char		 file[NFILEN];
	char		*cp, *conffile = NULL, *init_fcn_name = NULL;
	char		*batchfile = NULL;
	PF		 init_fcn = NULL;
	int	 	 o, i, nfiles;
	int	  	 nobackups = 0;
	struct buffer	*bp = NULL;

/* amiport: pledge/unveil stubbed as (0) in def.h */
#ifdef __OpenBSD__
	if (pledge("stdio rpath wpath cpath fattr chown getpw tty proc exec", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */
#endif

	while ((o = getopt(argc, argv, "hnRb:f:u:")) != -1)
		switch (o) {
		case 'b':
			batch = 1;
			batchfile = optarg;
			break;
		case 'R':
			allbro = 1;
			break;
		case 'n':
			nobackups = 1;
			break;
		case 'f':
			if (init_fcn_name != NULL)
				errx(10, "cannot specify more than one "
				    "initial function");
			init_fcn_name = optarg;
			break;
		case 'h':
			usage(0);
			break;
		case 'u':
			conffile = optarg;
			break;
		default:
			usage(10); /* amiport: RETURN_ERROR */
			break;
		}

	if (batch && (conffile != NULL)) {
                fprintf(stderr, "%s: -b and -u are mutually exclusive.\n",
                    PACKAGE_NAME);
                exit(10); /* amiport: RETURN_ERROR */
	}
	if (batch) {
		pty_init();
		conffile = batchfile;
	}
	if ((ffp = startupfile(NULL, conffile, file, sizeof(file))) == NULL &&
	    conffile != NULL) {
		fprintf(stderr, "%s: Problem with file: %s\n", PACKAGE_NAME,
		    conffile);
		exit(10); /* amiport: RETURN_ERROR */
	}

	argc -= optind;
	argv += optind;

#ifdef __AMIGA__
	setlocale(LC_ALL, "C"); /* amiport: force C locale — AmigaOS locale may return MB_CUR_MAX > 1 */
#else
	setlocale(LC_CTYPE, "");
#endif

	maps_init();		/* Keymaps and modes.		*/
	funmap_init();		/* Functions.			*/

#ifdef  MGLOG
	if (!mgloginit())
		errx(10, "Unable to create logging environment.");
#endif

	/*
	 * This is where we initialize standalone extensions that should
	 * be loaded dynamically sometime in the future.
	 */
	{
		extern void grep_init(void);
		extern void dired_init(void);
		extern void cmode_init(void);

#ifdef ENABLE_DIRED
		dired_init();
#endif
#ifdef ENABLE_COMPILE_GREP
		grep_init();
#endif
#ifdef ENABLE_CMODE
		cmode_init();
#endif
	}

	if (init_fcn_name &&
	    (init_fcn = name_function(init_fcn_name)) == NULL)
		errx(10, "Unknown function `%s'", init_fcn_name);

	vtinit();		/* Virtual terminal.		*/
	dirinit();		/* Get current directory.	*/
	edinit(bp);		/* Buffers, windows.		*/
	ttykeymapinit();	/* Symbols, bindings.		*/
	bellinit();		/* Audible and visible bell.	*/
	dblspace = 1;		/* two spaces for sentence end. */

	/*
	 * doing update() before reading files causes the error messages from
	 * the file I/O show up on the screen.	(and also an extra display of
	 * the mode line if there are files specified on the command line.)
	 */
	update(CMODE);

	/* user startup file. */
	if (ffp != NULL) {
		(void)load(ffp, file);
		ffclose(ffp, NULL);
	}

	if (batch) {
		vttidy();
		return (0);
	}

	/*
	 * Now ensure any default buffer modes from the startup file are
	 * given to any files opened when parsing the startup file.
	 * Note *scratch* will also be updated.
	 */
	for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
		bp->b_flag = defb_flag;
		for (i = 0; i <= defb_nmodes; i++) {
                	bp->b_modes[i] = defb_modes[i];
        	}
	}

	/* Force FFOTHARG=1 so that this mode is enabled, not simply toggled */
	if (init_fcn)
		init_fcn(FFOTHARG, 1);

	if (nobackups)
		makebkfile(FFARG, 0);

	for (nfiles = 0, i = 0; i < argc; i++) {
		if (argv[i][0] == '+' && strlen(argv[i]) >= 2) {
			long long lval;
			const char *errstr;

			lval = strtonum(&argv[i][1], INT_MIN, INT_MAX, &errstr);
			if (argv[i][1] == '\0' || errstr != NULL)
				goto notnum;
			startrow = lval;
		} else {
notnum:
			cp = adjustname(argv[i], FALSE);
			if (cp != NULL) {
				if (nfiles == 1)
					splitwind(0, 1);

				if (fisdir(cp) == TRUE) {
					(void)do_dired(cp);
					continue;
				}
				if ((curbp = findbuffer(cp)) == NULL) {
					vttidy();
					errx(10, "Can't find current buffer!");
				}
				(void)showbuffer(curbp, curwp, 0);
				if (readin(cp) != TRUE)
					killbuffer(curbp);
				else {
					/* Ensure enabled, not just toggled */
					if (init_fcn_name)
						init_fcn(FFOTHARG, 1);
					nfiles++;
				}
				if (allbro)
					curbp->b_flag |= BFREADONLY;
			}
		}
	}

	if (nfiles > 2)
		listbuffers(0, 1);

	ewprintf(" %s", hlp);

	/* fake last flags */
	thisflag = 0;
	for (;;) {
		if (epresf == KCLEAR)
			eerase();
		if (epresf == TRUE)
			epresf = KCLEAR;
		if (winch_flag) {
			do_redraw(0, 0, TRUE);
			winch_flag = 0;
		}
		update(CMODE);
		lastflag = thisflag;
		thisflag = 0;

		switch (doin()) {
		case TRUE:
			break;
		case ABORT:
			ewprintf("Quit");
			/* FALLTHRU */
		case FALSE:
		default:
			macrodef = FALSE;
		}
	}
}

/*
 * Initialize default buffer and window. Default buffer is called *scratch*.
 */
static void
edinit(struct buffer *bp)
{
	struct mgwin	*wp;

	bheadp = NULL;
	bp = bfind("*scratch*", TRUE);		/* Text buffer.          */
	if (bp == NULL)
		panic("edinit");

	wp = new_window(bp);
	if (wp == NULL)
		panic("edinit: Out of memory");

	curbp = bp;				/* Current buffer.	 */
	wheadp = wp;
	curwp = wp;
	wp->w_wndp = NULL;			/* Initialize window.	 */
	wp->w_linep = wp->w_dotp = bp->b_headp;
	wp->w_ntrows = nrow - 2;		/* 2 = mode, echo.	 */
	wp->w_rflag = WFMODE | WFFULL;		/* Full.		 */
}

/*
 * Create pty for batch mode.
 */
static void
pty_init(void)
{
#ifdef __AMIGA__
	/* amiport: openpty()/login_tty() not available on AmigaOS.
	 * Batch mode (-b) will use stdin/stdout directly.
	 * amiport-redesign: NEEDS HUMAN REVIEW if batch mode is required on Amiga */
	(void)0; /* no-op */
#else
	struct winsize	 ws;
	int		 master;
	int		 slave;

	memset(&ws, 0, sizeof(ws));
	ws.ws_col = 80,
	ws.ws_row = 24;

	openpty(&master, &slave, NULL, NULL, &ws);
	login_tty(slave);
#endif
	return;
}

/*
 * Quit command.  If an argument, always quit.  Otherwise confirm if a buffer
 * has been changed and not written out.  Normally bound to "C-x C-c".
 */
int
quit(int f, int n)
{
	int	 s;

	if ((s = anycb(FALSE)) == ABORT)
		return (ABORT);
	if (s == FIOERR || s == UERROR)
		return (FALSE);
	if (s == FALSE
	    || eyesno("Modified buffers exist; really exit") == TRUE) {
		vttidy();
#ifdef ENABLE_CTAGS
		closetags();
#endif
		exit(0);
	}
	return (TRUE);
}

/*
 * User abort.  Should be called by any input routine that sees a C-g to abort
 * whatever C-g is aborting these days. Currently does nothing.
 */
int
ctrlg(int f, int n)
{
	return (ABORT);
}
