/*
 * signal_emu.c — Minimal signal emulation for AmigaOS
 *
 * Only SIGINT has a real mapping (via SIGBREAKF_CTRL_C).
 * Other signals are stubbed.
 */

#include <amiport/signal.h>

#include <proto/exec.h>
#include <proto/dos.h>

/* Signal handler table */
static amiport_sighandler_t handlers[16] = { 0 };

amiport_sighandler_t amiport_signal(int signum, amiport_sighandler_t handler)
{
    amiport_sighandler_t old;

    if (signum < 1 || signum > 15) {
        return AMIPORT_SIG_ERR;
    }

    old = handlers[signum];
    handlers[signum] = handler;
    return old;
}

int amiport_raise(int signum)
{
    amiport_sighandler_t handler;

    if (signum < 1 || signum > 15) {
        return -1;
    }

    handler = handlers[signum];

    if (handler == AMIPORT_SIG_DFL) {
        /* Default action: ignore for most signals */
        return 0;
    }

    if (handler == AMIPORT_SIG_IGN) {
        return 0;
    }

    if (handler != NULL) {
        handler(signum);
    }

    return 0;
}

int amiport_check_break(void)
{
    ULONG signals;

    /* Check for Ctrl-C without blocking */
    signals = SetSignal(0L, 0L);

    if (signals & SIGBREAKF_CTRL_C) {
        /* Clear the signal */
        SetSignal(0L, SIGBREAKF_CTRL_C);

        /* Call SIGINT handler if set */
        if (handlers[AMIPORT_SIGINT] != AMIPORT_SIG_DFL &&
            handlers[AMIPORT_SIGINT] != AMIPORT_SIG_IGN &&
            handlers[AMIPORT_SIGINT] != NULL) {
            handlers[AMIPORT_SIGINT](AMIPORT_SIGINT);
        }

        return 1;
    }

    return 0;
}

/*
 * sigaction -- set/query signal action
 *
 * amiport: Only SIGINT (2) has a real mapping to AmigaOS Ctrl-C.
 * All other signals are accepted silently. The sa_mask and sa_flags
 * fields are stored but have no effect on AmigaOS.
 */
int amiport_sigaction(int sig, const struct amiport_sigaction *act,
                      struct amiport_sigaction *oact)
{
    if (sig < 1 || sig > 15) {
        return -1;
    }

    if (oact) {
        oact->sa_handler = handlers[sig];
        oact->sa_mask = 0;
        oact->sa_flags = 0;
    }

    if (act) {
        handlers[sig] = act->sa_handler;
    }

    return 0;
}

int amiport_sigemptyset(amiport_sigset_t *set)
{
    if (!set) return -1;
    *set = 0;
    return 0;
}

int amiport_sigaddset(amiport_sigset_t *set, int signo)
{
    if (!set || signo < 1 || signo > 31) return -1;
    *set |= (1UL << signo);
    return 0;
}

/*
 * sigprocmask -- no-op on AmigaOS
 *
 * amiport: AmigaOS has no concept of blocking signals at the
 * process level. Accept the call silently and return 0.
 */
int amiport_sigprocmask(int how, const amiport_sigset_t *set,
                        amiport_sigset_t *oset)
{
    static amiport_sigset_t current_mask = 0;

    (void)how;

    if (oset) {
        *oset = current_mask;
    }

    if (set) {
        switch (how) {
        case AMIPORT_SIG_BLOCK:
            current_mask |= *set;
            break;
        case AMIPORT_SIG_UNBLOCK:
            current_mask &= ~(*set);
            break;
        case AMIPORT_SIG_SETMASK:
            current_mask = *set;
            break;
        }
    }

    return 0;
}

/*
 * nanosleep -- sleep with nanosecond precision (approximated)
 *
 * amiport: Uses Delay() with 20ms (1 tick) granularity.
 * Nanosecond precision is not achievable on AmigaOS without
 * timer.device, but Delay() covers the common use case.
 * If rem is non-NULL, it is set to zero (no early wakeup).
 */
int amiport_nanosleep(const struct amiport_timespec *req,
                      struct amiport_timespec *rem)
{
    ULONG ticks;

    if (!req) return -1;

    /* Convert seconds + nanoseconds to ticks (1 tick = 20ms) */
    ticks = (ULONG)req->tv_sec * 50;
    ticks += (ULONG)(req->tv_nsec / 20000000L); /* 20ms per tick */

    /* Minimum 1 tick if any time was requested */
    if (ticks == 0 && (req->tv_sec > 0 || req->tv_nsec > 0)) {
        ticks = 1;
    }

    if (ticks > 0) {
        Delay(ticks);
    }

    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }

    return 0;
}
