/*
 * amiport/signal.h — Minimal signal emulation for AmigaOS
 *
 * Only SIGINT is meaningfully supported (via SIGBREAKF_CTRL_C).
 * Other signals are stubbed.
 */

#ifndef AMIPORT_SIGNAL_H
#define AMIPORT_SIGNAL_H

/* Signal numbers (matching common POSIX values) */
#define AMIPORT_SIGHUP     1
#define AMIPORT_SIGINT     2
#define AMIPORT_SIGQUIT    3
#define AMIPORT_SIGTERM   15
#define AMIPORT_SIGPIPE   13

/* Signal handler type */
typedef void (*amiport_sighandler_t)(int);

/* Special handler values */
#define AMIPORT_SIG_DFL   ((amiport_sighandler_t)0)
#define AMIPORT_SIG_IGN   ((amiport_sighandler_t)1)
#define AMIPORT_SIG_ERR   ((amiport_sighandler_t)-1)

/* Set signal handler. Returns previous handler or SIG_ERR. */
amiport_sighandler_t amiport_signal(int signum, amiport_sighandler_t handler);

/* Send signal to self */
int amiport_raise(int signum);

/* Check for pending Ctrl-C (Amiga-specific convenience) */
int amiport_check_break(void);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_SIGNAL_MACROS
#define SIGHUP    AMIPORT_SIGHUP
#define SIGINT    AMIPORT_SIGINT
#define SIGQUIT   AMIPORT_SIGQUIT
#define SIGTERM   AMIPORT_SIGTERM
#define SIGPIPE   AMIPORT_SIGPIPE
#define SIG_DFL   AMIPORT_SIG_DFL
#define SIG_IGN   AMIPORT_SIG_IGN
#define SIG_ERR   AMIPORT_SIG_ERR
#define signal    amiport_signal
#define raise     amiport_raise
#endif

#endif /* AMIPORT_SIGNAL_H */
