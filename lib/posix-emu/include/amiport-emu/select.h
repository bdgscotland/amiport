/*
 * amiport-emu/select.h — select()/poll() emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * - No true level-triggered readiness notification — uses polling
 * - Granularity limited to ~20ms (1 tick) minimum
 * - Does not work with socket fds (use bsdsocket.library WaitSelect())
 * - exceptfds parameter is ignored
 *
 * See docs/posix-tiers.md "Tier 2 — select()/poll()" for full details.
 */

#ifndef AMIPORT_EMU_SELECT_H
#define AMIPORT_EMU_SELECT_H

#ifndef AMIPORT_EMU_SELECT_ENABLED
#define AMIPORT_EMU_SELECT_ENABLED 1
#endif

#if AMIPORT_EMU_SELECT_ENABLED

/* Forward declaration to avoid pulling in full sys/time.h */
struct amiport_timeval;

/* Emulated fd_set — supports up to 32 descriptors */
#define AMIPORT_EMU_FD_SETSIZE 32

typedef struct {
    unsigned long fds_bits[(AMIPORT_EMU_FD_SETSIZE + 31) / 32];
} amiport_emu_fd_set;

#define AMIPORT_EMU_FD_ZERO(set)   do { (set)->fds_bits[0] = 0UL; } while(0)
#define AMIPORT_EMU_FD_SET(fd,set) do { (set)->fds_bits[0] |= (1UL << (fd)); } while(0)
#define AMIPORT_EMU_FD_CLR(fd,set) do { (set)->fds_bits[0] &= ~(1UL << (fd)); } while(0)
#define AMIPORT_EMU_FD_ISSET(fd,set) ((set)->fds_bits[0] & (1UL << (fd)))

/*
 * amiport_emu_select() — Emulated select() using WaitForChar() polling.
 *
 * nfds:     Highest fd + 1
 * readfds:  Set of fds to check for read readiness (may be NULL)
 * writefds: Set of fds to check for write readiness (may be NULL)
 * exceptfds: IGNORED on AmigaOS
 * timeout:  Maximum time to wait (NULL = block indefinitely)
 *
 * Returns: Number of ready fds, 0 on timeout, -1 on error
 */
int amiport_emu_select(int nfds,
                       amiport_emu_fd_set *readfds,
                       amiport_emu_fd_set *writefds,
                       amiport_emu_fd_set *exceptfds,
                       struct amiport_timeval *timeout);

#endif /* AMIPORT_EMU_SELECT_ENABLED */

#endif /* AMIPORT_EMU_SELECT_H */
