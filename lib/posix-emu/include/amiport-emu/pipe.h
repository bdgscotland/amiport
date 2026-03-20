/*
 * amiport-emu/pipe.h — Basic pipe() emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * - Uses PIPE: device (named pipe, not anonymous)
 * - Blocking/buffering behaviour differs from POSIX anonymous pipes
 * - EOF detection is less reliable
 * - No SIGPIPE on write to closed read end
 * - Requires AmigaOS 2.0+ with PIPE: handler installed
 *
 * See docs/posix-tiers.md "Tier 2 — pipe()" for full details.
 */

#ifndef AMIPORT_EMU_PIPE_H
#define AMIPORT_EMU_PIPE_H

#ifndef AMIPORT_EMU_PIPE_ENABLED
#define AMIPORT_EMU_PIPE_ENABLED 1
#endif

#if AMIPORT_EMU_PIPE_ENABLED

/*
 * amiport_emu_pipe() — Create a pipe using PIPE: device.
 *
 * pipefd: Array of two ints. pipefd[0] is the read end, pipefd[1] is write.
 *
 * Returns: 0 on success, -1 on error (sets errno)
 */
int amiport_emu_pipe(int pipefd[2]);

#endif /* AMIPORT_EMU_PIPE_ENABLED */

#endif /* AMIPORT_EMU_PIPE_H */
