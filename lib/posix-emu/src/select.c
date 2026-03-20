/*
 * select.c — select() emulation via WaitForChar() polling
 *
 * Tier 2 emulation: approximate POSIX semantics with documented differences.
 * See amiport-emu/select.h for the emulation notice.
 */

#include <amiport-emu/select.h>

#if AMIPORT_EMU_SELECT_ENABLED

#include <amiport/sys/time.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* Minimum poll interval in microseconds (1 Amiga tick = 20ms) */
#define POLL_INTERVAL_US 20000

/* Access the Tier 1 shim fd table to get BPTR file handles */
extern BPTR amiport_fd_to_fh(int fd);

int amiport_emu_select(int nfds,
                       amiport_emu_fd_set *readfds,
                       amiport_emu_fd_set *writefds,
                       amiport_emu_fd_set *exceptfds,
                       struct amiport_timeval *timeout)
{
    long timeout_us;
    long elapsed_us = 0;
    int ready = 0;
    int i;
    amiport_emu_fd_set orig_readfds;

    (void)exceptfds; /* ignored on AmigaOS */

    /* Calculate total timeout in microseconds */
    if (timeout) {
        timeout_us = (timeout->tv_sec * 1000000L) + timeout->tv_usec;
    } else {
        timeout_us = -1; /* block indefinitely */
    }

    /* Save original sets and clear result sets */
    if (readfds) {
        orig_readfds = *readfds;
        AMIPORT_EMU_FD_ZERO(readfds);
    }

    /* Write fds are always considered ready on AmigaOS
     * (no non-blocking write support in the emulation) */
    if (writefds) {
        for (i = 0; i < nfds && i < AMIPORT_EMU_FD_SETSIZE; i++) {
            if (AMIPORT_EMU_FD_ISSET(i, writefds)) {
                ready++;
            }
        }
    }

    /* Poll loop for read readiness */
    do {
        if (readfds) {
            for (i = 0; i < nfds && i < AMIPORT_EMU_FD_SETSIZE; i++) {
                BPTR fh;

                if (!AMIPORT_EMU_FD_ISSET(i, &orig_readfds)) {
                    continue;
                }

                fh = amiport_fd_to_fh(i);
                if (fh == 0) {
                    continue;
                }

                /* WaitForChar with 0 timeout = non-blocking check */
                if (WaitForChar(fh, 0)) {
                    AMIPORT_EMU_FD_SET(i, readfds);
                    ready++;
                }
            }
        }

        if (ready > 0) {
            break;
        }

        /* No fds ready yet — sleep one tick and retry */
        if (timeout_us == 0) {
            break; /* non-blocking poll */
        }

        Delay(1); /* 1 tick = ~20ms */
        elapsed_us += POLL_INTERVAL_US;

        /* Check for Ctrl-C */
        if (SetSignal(0L, 0L) & SIGBREAKF_CTRL_C) {
            return -1;
        }

    } while (timeout_us < 0 || elapsed_us < timeout_us);

    return ready;
}

#endif /* AMIPORT_EMU_SELECT_ENABLED */
