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
