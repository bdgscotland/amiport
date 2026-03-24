/*
 * amiport-console/term.h — Terminal capability stubs for AmigaOS
 *
 * Drop-in replacement for <term.h>. Most programs only need setupterm()
 * and a few capability queries. We stub these since Amiga console.device
 * has fixed capabilities.
 */

#ifndef AMIPORT_CONSOLE_TERM_H
#define AMIPORT_CONSOLE_TERM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Terminal setup — always succeeds on Amiga */
int setupterm(const char *term, int fildes, int *errret);

/* Capability queries — return fixed values for Amiga console */
int  tigetnum(const char *capname);
int  tigetflag(const char *capname);
char *tigetstr(const char *capname);

/* tputs — output a terminal string with padding */
int tputs(const char *str, int affcnt, int (*putc_func)(int));

/* tparm — parameterize a terminfo capability string
 * Processes: %d (decimal), %i (increment), %p1/%p2 (select param), %% (literal) */
char *tparm(const char *str, ...);

/* ================================================================
 * Termcap API — 2-letter capability codes
 *
 * Used by less, vi, and other programs that use termcap directly
 * rather than ncurses. Returns fixed values for Amiga console.device.
 * ================================================================ */

/* Load terminal entry — always succeeds on Amiga (returns 1) */
int tgetent(char *bp, const char *name);

/* Query numeric capability (2-letter code: "co"=cols, "li"=lines) */
int tgetnum(const char *cap);

/* Query boolean capability (2-letter code: "am"=auto-margins) */
int tgetflag(const char *cap);

/* Query string capability (2-letter code: "cm"=cursor-motion, "ce"=clear-eol)
 * If area is non-NULL, writes into *area and advances the pointer.
 * If area is NULL, returns pointer to static buffer. */
char *tgetstr(const char *cap, char **area);

/* Format cursor motion string — substitutes col and row into cm capability */
char *tgoto(const char *cm, int col, int row);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_CONSOLE_TERM_H */
