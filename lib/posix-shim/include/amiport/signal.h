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

/*
 * sigaction -- POSIX signal action stubs for AmigaOS
 *
 * amiport: AmigaOS has no POSIX signals beyond SIGINT (Ctrl-C).
 * sigaction() stores the handler for SIGINT; all other signals
 * are accepted but have no effect. sigprocmask() is a no-op
 * (cannot block signals on AmigaOS). sigemptyset/sigaddset
 * manipulate a bitmask but it has no real effect.
 *
 * This satisfies code like find(1) and pr(1) that call sigaction()
 * for graceful signal handling.
 */

/* Signal set type -- just a bitmask */
typedef unsigned long amiport_sigset_t;

/* sigaction structure */
struct amiport_sigaction {
    void (*sa_handler)(int);    /* signal handler */
    amiport_sigset_t sa_mask;   /* signals to block during handler */
    int sa_flags;               /* flags (ignored on AmigaOS) */
};

/* sigaction flags (accepted but ignored) */
#define AMIPORT_SA_RESTART  0x0002
#define AMIPORT_SA_NOCLDSTOP 0x0008

/* sigprocmask how values */
#define AMIPORT_SIG_BLOCK   0
#define AMIPORT_SIG_UNBLOCK 1
#define AMIPORT_SIG_SETMASK 2

int amiport_sigaction(int sig, const struct amiport_sigaction *act,
                      struct amiport_sigaction *oact);
int amiport_sigemptyset(amiport_sigset_t *set);
int amiport_sigaddset(amiport_sigset_t *set, int signo);
int amiport_sigprocmask(int how, const amiport_sigset_t *set,
                        amiport_sigset_t *oset);

/* nanosleep -- high-resolution sleep via Delay()
 * Uses struct timespec from sys/_timespec.h if available,
 * otherwise defines amiport_timespec as a compatible struct. */
#include <time.h>  /* pulls in sys/_timespec.h on bebbo-gcc */

int amiport_nanosleep(const struct timespec *req,
                      struct timespec *rem);

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

#define sigset_t         amiport_sigset_t
#define sigaction        amiport_sigaction
#define sigemptyset(s)   amiport_sigemptyset(s)
#define sigaddset(s, n)  amiport_sigaddset(s, n)
#define sigprocmask(h, s, o) amiport_sigprocmask(h, s, o)
#define SA_RESTART       AMIPORT_SA_RESTART
#define SA_NOCLDSTOP     AMIPORT_SA_NOCLDSTOP
#define SIG_BLOCK        AMIPORT_SIG_BLOCK
#define SIG_UNBLOCK      AMIPORT_SIG_UNBLOCK
#define SIG_SETMASK      AMIPORT_SIG_SETMASK

/* amiport: Do NOT define timespec macro -- bebbo-gcc libnix provides
 * struct timespec in sys/_timespec.h. Our amiport_timespec is compatible
 * but redefining would cause conflicts. Ports use struct timespec
 * from system headers or struct amiport_timespec from this header. */
#define nanosleep(r, m)  amiport_nanosleep(r, m)
#endif

#endif /* AMIPORT_SIGNAL_H */
