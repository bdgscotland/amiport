/*	$OpenBSD: screen.c,v 1.19 2019/06/28 13:32:52 deraadt Exp $	*/
/*	$NetBSD: screen.c,v 1.4 1995/04/29 01:11:36 mycroft Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)screen.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Tetris screen control -- AmigaOS port.
 */

/* amiport: removed #include <sys/ioctl.h> -- TIOCGWINSZ provided by amiport/unistd.h */
/* amiport: removed #include <setjmp.h> -- no SIGTSTP/setjmp needed on AmigaOS */

/* amiport: removed bare <err.h> -- use <amiport/err.h> below (no libnix err.h) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* amiport: replaced <term.h> with console-shim termcap (ADR-009) */
#include <amiport-console/term.h>
/* amiport: replaced <signal.h> with amiport shim */
#include <amiport/signal.h>
/* amiport: added amiport/unistd.h for TIOCGWINSZ and amiport_ioctl() */
#include <amiport/unistd.h>
/* amiport: added amiport/termios.h for tcgetattr/tcsetattr via SetMode() */
#include <amiport/termios.h>
#include <amiport/err.h>

#include "screen.h"
#include "tetris.h"

static cell curscreen[B_SIZE];	/* 1 => standout (or otherwise marked) */
static int curscore;
static int isset;		/* true => terminal is in game mode */
static struct termios oldtt;

/* amiport: removed static void scr_stop(int) -- no SIGTSTP on AmigaOS */
/* amiport: removed static void stopset(int) -- no SIGTSTP/setjmp on AmigaOS */
/* amiport: removed static void (*tstp)(int) -- no SIGTSTP handler needed */

/*
 * Capabilities from TERMCAP.
 */
extern char	PC, *BC, *UP;		/* tgoto requires globals: ugh! */

static char
	*bcstr,			/* backspace char */
	*CEstr,			/* clear to end of line */
	*CLstr,			/* clear screen */
	*CMstr,			/* cursor motion string */
#ifdef unneeded
	*CRstr,			/* "\r" equivalent */
#endif
	*HOstr,			/* cursor home */
	*LLstr,			/* last line, first column */
	*pcstr,			/* pad character */
	*TEstr,			/* end cursor motion mode */
	*TIstr,			/* begin cursor motion mode */
	*VIstr,			/* make cursor invisible */
	*VEstr;			/* make cursor appear normal */
char
	*SEstr,			/* end standout mode */
	*SOstr;			/* begin standout mode */
static int
	COnum,			/* co# value */
	LInum,			/* li# value */
	MSflag;			/* can move in standout mode */


struct tcsinfo {		/* termcap string info; some abbrevs above */
	char tcname[3];
	char **tcaddr;
} tcstrings[] = {
	{"bc", &bcstr},
	{"ce", &CEstr},
	{"cl", &CLstr},
	{"cm", &CMstr},
#ifdef unneeded
	{"cr", &CRstr},
#endif
	{"le", &BC},		/* move cursor left one space */
	{"pc", &pcstr},
	{"se", &SEstr},
	{"so", &SOstr},
	{"te", &TEstr},
	{"ti", &TIstr},
	{"vi", &VIstr},
	{"ve", &VEstr},
	{"up", &UP},		/* cursor up */
	{ {0}, NULL}
};

/* This is where we will actually stuff the information */

static char combuf[1024], tbuf[1024];


/*
 * Routine used by tputs().
 */
int
put(int c)
{

	return (putchar(c));
}

/*
 * putstr() is for unpadded strings (either as in termcap(5) or
 * simply literal strings); putpad() is for padded strings with
 * count=1.  (See screen.h for putpad().)
 */
#define	putstr(s)	(void)fputs(s, stdout)
#define	moveto(r, c)	putpad(tgoto(CMstr, c, r))

/*
 * Set up from termcap.
 * amiport: console-shim tgetent() always returns 1 (success).
 * amiport: getenv("TERM") falls back to "amiga" if not set.
 */
void
scr_init(void)
{
	static int bsflag, xsflag, sgnum;
#ifdef unneeded
	static int ncflag;
#endif
	char *term, *fill;
	static struct tcninfo {	/* termcap numeric and flag info */
		char tcname[3];
		int *tcaddr;
	} tcflags[] = {
		{"bs", &bsflag},
		{"ms", &MSflag},
#ifdef unneeded
		{"nc", &ncflag},
#endif
		{"xs", &xsflag},
		{ {0}, NULL}
	}, tcnums[] = {
		{"co", &COnum},
		{"li", &LInum},
		{"sg", &sgnum},
		{ {0}, NULL}
	};

	/* amiport: getenv("TERM") -- use "amiga" as fallback on AmigaOS */
	term = getenv("TERM");
	if (term == NULL || *term == '\0') {
#ifdef __AMIGA__
		term = "amiga";
#else
		stop("you must set the TERM environment variable");
#endif
	}
	/* amiport: tgetent() from console-shim always returns 1 -- no error path */
	if (tgetent(tbuf, term) <= 0)
		stop("cannot find your termcap");
	fill = combuf;
	{
		struct tcsinfo *p;

		for (p = tcstrings; p->tcaddr; p++)
			*p->tcaddr = tgetstr(p->tcname, &fill);
	}
	if (classic)
		SOstr = SEstr = NULL;
	{
		struct tcninfo *p;

		for (p = tcflags; p->tcaddr; p++)
			*p->tcaddr = tgetflag(p->tcname);
		for (p = tcnums; p->tcaddr; p++)
			*p->tcaddr = tgetnum(p->tcname);
	}
	if (bsflag)
		BC = "\b";
	else if (BC == NULL && bcstr != NULL)
		BC = bcstr;
	if (CLstr == NULL)
		stop("cannot clear screen");
	if (CMstr == NULL || UP == NULL || BC == NULL)
		stop("cannot do random cursor positioning via tgoto()");
	PC = pcstr ? *pcstr : 0;
	if (sgnum > 0 || xsflag)
		SOstr = SEstr = NULL;
#ifdef unneeded
	if (ncflag)
		CRstr = NULL;
	else if (CRstr == NULL)
		CRstr = "\r";
#endif
}

/*
 * Set up screen mode.
 * amiport: removed SIGTSTP/SIGTTOU job control -- AmigaOS has no job control.
 * amiport: removed setjmp/longjmp -- not needed without SIGTSTP.
 * amiport: removed sigprocmask() calls -- no signal masking needed.
 * amiport: TIOCGWINSZ via amiport_ioctl() + amiport_winsize struct.
 * amiport: OXTABS guarded -- not defined in amiport/termios.h.
 */
void
scr_set(void)
{
	struct winsize ws;
	struct termios newtt;

	/* amiport: no SIGTSTP/SIGTTOU blocking needed on AmigaOS */
	Rows = 0;
	Cols = 0;
	/* amiport: replaced ioctl(0,...) with amiport_ioctl() */
	if (amiport_ioctl(0, TIOCGWINSZ, &ws) == 0) {
		Rows = ws.ws_row;
		Cols = ws.ws_col;
	}
	if (Rows == 0)
		Rows = LInum;
	if (Cols == 0)
		Cols = COnum;
	if (Rows < MINROWS || Cols < MINCOLS) {
		char smallscr[55];

		(void)snprintf(smallscr, sizeof(smallscr),
		    "the screen is too small (must be at least %dx%d)",
		    MINROWS, MINCOLS);
		stop(smallscr);
	}
	if (tcgetattr(0, &oldtt) == -1)
		stop("tcgetattr() fails");
	newtt = oldtt;
	newtt.c_lflag &= ~(ICANON|ECHO);
	/* amiport: OXTABS not defined in amiport/termios.h -- guarded */
#ifdef OXTABS
	newtt.c_oflag &= ~OXTABS;
#endif
	if (tcsetattr(0, TCSADRAIN, &newtt) == -1)
		stop("tcsetattr() fails");

	/*
	 * We made it.  We are now in screen mode, modulo TIstr
	 * (which we will fix immediately).
	 */
	if (TIstr)
		putstr(TIstr);	/* termcap(5) says this is not padded */
	if (VIstr)
		putstr(VIstr);	/* termcap(5) says this is not padded */

	/* amiport: no SIGTSTP handler installation -- AmigaOS has no job control */

	isset = 1;
	scr_clear();
}

/*
 * End screen mode.
 * amiport: removed SIGTSTP/SIGTTOU blocking and signal restoration.
 */
void
scr_end(void)
{
	/* amiport: no signal masking needed on AmigaOS */

	/* move cursor to last line */
	if (LLstr)
		putstr(LLstr);	/* termcap(5) says this is not padded */
	else
		moveto(Rows - 1, 0);
	/* exit screen mode */
	if (TEstr)
		putstr(TEstr);	/* termcap(5) says this is not padded */
	if (VEstr)
		putstr(VEstr);	/* termcap(5) says this is not padded */
	(void) fflush(stdout);
	(void) tcsetattr(0, TCSADRAIN, &oldtt);
	isset = 0;
	/* amiport: no SIGTSTP signal restoration needed */
}

void
stop(char *why)
{

	if (isset)
		scr_end();
	/* amiport: exit code 1 -> 10 (RETURN_ERROR) */
	errx(10, "aborting: %s", why);
}

/*
 * Clear the screen, forgetting the current contents in the process.
 */
void
scr_clear(void)
{

	putpad(CLstr);
	curscore = -1;
	memset((char *)curscreen, 0, sizeof(curscreen));
}

typedef cell regcell;

/*
 * Update the screen.
 * amiport: removed SIGTSTP masking in sigprocmask() calls.
 */
void
scr_update(void)
{
	cell *bp, *sp;
	regcell so, cur_so = 0;
	int i, ccol, j;
	/* amiport: removed sigset_t -- no signal masking on AmigaOS */
	static const struct shape *lastshape;

	/* amiport: no SIGTSTP masking needed */

	/* always leave cursor after last displayed point */
	curscreen[D_LAST * B_COLS - 1] = -1;

	if (score != curscore) {
		if (HOstr)
			putpad(HOstr);
		else
			moveto(0, 0);
		(void) printf("Score: %d", score);
		curscore = score;
	}

	/* draw preview of next pattern */
	if (showpreview && (nextshape != lastshape)) {
		static int r=5, c=2;
		int tr, tc, t;

		lastshape = nextshape;

		/* clean */
		putpad(SEstr);
		moveto(r-1, c-1); putstr("          ");
		moveto(r,   c-1); putstr("          ");
		moveto(r+1, c-1); putstr("          ");
		moveto(r+2, c-1); putstr("          ");

		moveto(r-3, c-2);
		putstr("Next shape:");

		/* draw */
		if (SOstr)
			putpad(SOstr);
		moveto(r, 2 * c);
		putstr(SOstr ? "  " : "[]");
		for (i = 0; i < 3; i++) {
			t = c + r * B_COLS;
			t += nextshape->off[i];

			tr = t / B_COLS;
			tc = t % B_COLS;

			moveto(tr, 2*tc);
			putstr(SOstr ? "  " : "[]");
		}
		putpad(SEstr);
	}

	bp = &board[D_FIRST * B_COLS];
	sp = &curscreen[D_FIRST * B_COLS];
	for (j = D_FIRST; j < D_LAST; j++) {
		ccol = -1;
		for (i = 0; i < B_COLS; bp++, sp++, i++) {
			if (*sp == (so = *bp))
				continue;
			*sp = so;
			if (i != ccol) {
				if (cur_so && MSflag) {
					putpad(SEstr);
					cur_so = 0;
				}
				moveto(RTOD(j), CTOD(i));
			}
			if (SOstr) {
				if (so != cur_so) {
					putpad(so ? SOstr : SEstr);
					cur_so = so;
				}
				putstr("  ");
			} else
				putstr(so ? "[]" : "  ");
			ccol = i + 1;
			/*
			 * Look ahead a bit, to avoid extra motion if
			 * we will be redrawing the cell after the next.
			 * Motion probably takes four or more characters,
			 * so we save even if we rewrite two cells
			 * `unnecessarily'.  Skip it all, though, if
			 * the next cell is a different color.
			 */
#define	STOP (B_COLS - 3)
			if (i > STOP || sp[1] != bp[1] || so != bp[1])
				continue;
			if (sp[2] != bp[2])
				sp[1] = -1;
			else if (i < STOP && so == bp[2] && sp[3] != bp[3]) {
				sp[2] = -1;
				sp[1] = -1;
			}
		}
	}
	if (cur_so)
		putpad(SEstr);
	(void) fflush(stdout);
	/* amiport: no SIGTSTP unmasking needed */
}

/*
 * Write a message (set!=0), or clear the same message (set==0).
 * (We need its length in case we have to overwrite with blanks.)
 */
void
scr_msg(char *s, int set)
{

	if (set || CEstr == NULL) {
		int l = strlen(s);

		moveto(Rows - 2, ((Cols - l) >> 1) - 1);
		if (set)
			putstr(s);
		else
			while (--l >= 0)
				(void) putchar(' ');
	} else {
		moveto(Rows - 2, 0);
		putpad(CEstr);
	}
}
