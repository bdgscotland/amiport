/* defines.h — AmigaOS 3.x configuration for less 692
 *
 * Hand-crafted for amiport. Based on defines.h.in from less 692.
 * amiport: hand-crafted defines.h for AmigaOS (replaces configure)
 */

/* User preferences. */

#define SECURE_COMPILE  0
#define SECURE          SECURE_COMPILE

#define SHELL_ESCAPE    (!SECURE)
#define EXAMINE         (!SECURE)
#define TAB_COMPLETE_FILENAME (!SECURE)
#define CMD_HISTORY     1
#define HILITE_SEARCH   1
#define EDITOR          (!SECURE)
#define TAGS            (!SECURE)
#define USERFILE        (!SECURE)
#define GLOB            (!SECURE)
#define PIPEC           (!SECURE && HAVE_POPEN)
#define LOGFILE         (!SECURE)
#define OSC8_LINK       1
#define GNU_OPTIONS     1
#define ONLY_RETURN     0

/* amiport: Use Amiga-style paths for config/history files.
 * HOME is typically "S:" on AmigaOS. */
#define LESSKEYFILE             ".less"
#define LESSKEYFILE_SYS         "S:sysless"
#define DEF_LESSKEYINFILE       ".lesskey"
#define LESSKEYINFILE_SYS       "S:syslesskey"
#define LESSHISTFILE            ".lesshst"

/* Not MSDOS, not OS2 */
#define MSDOS_COMPILER  0
#define OS2             0

/* amiport: AmigaOS uses "/" as directory separator in paths */
#define PATHNAME_SEP    "/"

/* Settings always true on Unix (and AmigaOS via libnix). */

#define TGETENT_OK  1
#define HAVE_ANSI_PROTOS 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_PERROR     1
#define HAVE_TIME       1
#define HAVE_SHELL      1

/* amiport: AmigaOS shell metacharacters */
#define DEF_METACHARS   "; *?\t\n'\"()<>[]|&^`#\\$%=~{},"
#define DEF_METAESCAPE  "\\"

#define HAVE_DUP        1
#define HAVE_MEMCPY     1
#define HAVE_STRCHR     1
#define HAVE_STRSTR     1
#define HAVE_LESSKEYSRC 1

/* Buffer sizes — use smaller values for Amiga memory constraints */
#define CMDBUF_SIZE     2048
#define UNGOT_SIZE      200
#define LINEBUF_SIZE    1024
#define OUTBUF_SIZE     4096    /* amiport: was 1024 — fewer write() calls to console.device */
#define PROMPT_SIZE     2048
#define TERMBUF_SIZE    2048
#define TERMSBUF_SIZE   1024
#define TAGLINE_SIZE    1024
#define TABSTOP_MAX     128
#define LINENUM_POOL    1024

#define RETSIGTYPE void

/* amiport: Editor — use "ed" which is available on AmigaOS */
#define EDIT_PGM        "ed"

/* amiport: System directory for less config */
#define SYSDIR          "S:"

/* Settings for AmigaOS via libnix */

#define HAVE_CONST      1
#define HAVE_VOID       1
#define HAVE_CTYPE_H    1
#define HAVE_ERRNO      1
#define HAVE_ERRNO_H    1
#define HAVE_FCNTL_H    1
#define HAVE_FILENO     1
#define HAVE_LIMITS_H   1
#define HAVE_STDIO_H    1
#define HAVE_STDLIB_H   1
#define HAVE_STRERROR   1
#define HAVE_STRING_H   1
#define HAVE_TIME_H     1
#define HAVE_TIME_T     1
#define HAVE_UNISTD_H   1
#define HAVE_UPPER_LOWER 1
#define HAVE_SETTABLE_ERRNO 1
#define HAVE_STAT       1
#define HAVE_SYSTEM     1
#define HAVE_POPEN      0       /* amiport: popen/pclose not in libnix — disable preprocessor pipes */

/* amiport: termcap via console-shim */
#define HAVE_TERMCAP_H  1
#define USE_TERMCAP     1

/* amiport: termios via posix-shim */
#define HAVE_TERMIOS_H  1
#define HAVE_TERMIOS_FUNCS 1

/* amiport: Use less's bundled V8 regexp (not POSIX regex) */
#define HAVE_V8_REGCOMP 1

/* amiport: snprintf available in libnix (but NOT C99-compliant NULL,0 usage) */
#define HAVE_SNPRINTF   1

/* Features NOT available on AmigaOS */
#define HAVE_FCHMOD     0
#define HAVE_FSYNC      0
#define HAVE_GNU_REGEX  0
#define HAVE_LOCALE     0       /* amiport: Tier 2 — disabled, latin1 default */
#define HAVE_NANOSLEEP  0       /* amiport: Tier 2 — disabled, sleep() fallback */
#define HAVE_NCURSESW_TERMCAP_H 0
#define HAVE_NCURSES_TERMCAP_H  0
#define HAVE_OSPEED     0
#define HAVE_PCRE       0
#define HAVE_PCRE2      0
#define HAVE_POLL       0       /* amiport: Tier 2 — disabled, blocking read fallback */
#define HAVE_POSIX_REGCOMP 0
#define HAVE_REALPATH   0
#define HAVE_REGCMP     0
#define HAVE_REGEXEC2   0
#define HAVE_RE_COMP    0
#define HAVE_SGSTAT_H   0
#define HAVE_SIGEMPTYSET 0
#define HAVE_SIGPROCMASK 0
#define HAVE_SIGSETJMP  0
#define HAVE_SIGSETMASK 0
#define HAVE_SIGSET_T   0
#define HAVE_STAT_INO   0      /* AmigaOS does not have inode numbers */
#define HAVE_STDCKDINT_H 0
#define HAVE_STDINT_H   0      /* Not available in libnix (C99) */
#define HAVE_STRSIGNAL  0
#define HAVE_SYS_ERRLIST 0
#define HAVE_SYS_IOCTL_H 0    /* amiport: Tier 2 — no ioctl on AmigaOS */
#define HAVE_SYS_PTEM_H 0
#define HAVE_SYS_STREAM_H 0
#define HAVE_SYS_WAIT_H 0
#define HAVE_TERMINFO   0
#define HAVE_TERMIO_H   0
#define HAVE_TTYNAME    0      /* amiport: Tier 2 — no /dev/tty, use "*" */
#define HAVE_USLEEP     0      /* amiport: Tier 2 — disabled */
#define HAVE_VALUES_H   0
#define HAVE__SETJMP    0
#define HAVE_WCTYPE     0      /* No wide char support on AmigaOS */

#define MUST_DEFINE_ERRNO  0
#define MUST_DEFINE_OSPEED 0
#define NO_REGEX        0      /* We have V8 regex */
#define STAT_MACROS_BROKEN 0
#define STDC_HEADERS    1
#define LESSTEST        0

/* Package info */
#define PACKAGE_NAME    "less"
#define PACKAGE_VERSION "692"
#define PACKAGE_STRING  "less 692"
#define PACKAGE_TARNAME "less"
#define PACKAGE_URL     "https://www.greenwoodsoftware.com/less/"
#define PACKAGE_BUGREPORT "bug-less@gnu.org"
