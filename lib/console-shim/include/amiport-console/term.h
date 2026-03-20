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

/* tparm — parameterize a capability string (simplified) */
char *tparm(const char *str, ...);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_CONSOLE_TERM_H */
