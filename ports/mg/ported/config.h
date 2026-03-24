/* config.h — AmigaOS build configuration for mg
 * amiport: Hand-crafted to replace autoconf-generated config.h
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Package identity */
#define PACKAGE_NAME "mg"
#define PACKAGE_STRING "Mg 3.7"
#define PACKAGE_VERSION "3.7"

/* Features enabled */
#define WITHOUT_CURSES 1      /* Use built-in ANSI backend (ansi.c) */
#define REGEX 1               /* Enable regex search support */
#define ENABLE_AUTOEXEC 1     /* Enable auto-exec support */
#define ENABLE_CMODE 1        /* Enable C programming mode */
#define ENABLE_CTAGS 1        /* Enable ctags support */
#define TOGGLENL 1            /* Enable toggle-newline-prompt */

/* Features DISABLED on AmigaOS (require fork/exec) */
/* #undef ENABLE_COMPILE_GREP */
/* #undef CSCOPE */
/* #undef ENABLE_DIRED */

/* Startup file — look for S:.mg on AmigaOS */
#ifdef __AMIGA__
#define STARTUPFILE "S:.mg"
#else
#define STARTUPFILE "/usr/share/mg/mg"
#endif

/* Functions available in libnix */
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1
#define HAVE_REALLOCARRAY 1

/* strtonum provided by amiport/err.h shim */
#define HAVE_STRTONUM 1

/* amiport: fparseln bundled as ported/fparseln.c — do NOT define HAVE_FPARSELN
 * The FPARSELN_* flag macros in def.h are gated on !HAVE_FPARSELN */
/* #undef HAVE_FPARSELN */

/* NOT available on AmigaOS */
/* #undef HAVE_LOGIN_TTY */
/* #undef HAVE_OPENPTY */
/* #undef HAVE_FUTIMENS */
/* #undef HAVE_PTY_H */
/* #undef HAVE_UTMP_H */
/* #undef HAVE_NCURSES_CURSES_H */

/* No debug logging */
/* #undef MGLOG */

/* Paths for tutorial/help files */
#ifdef __AMIGA__
#define DATADIR "PROGDIR:data"
#define DOCDIR  "PROGDIR:doc"
#else
#define DATADIR "/usr/local/share/mg"
#define DOCDIR  "/usr/local/share/doc/mg"
#endif

#endif /* CONFIG_H */
