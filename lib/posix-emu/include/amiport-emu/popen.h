/*
 * amiport-emu/popen.h -- popen/pclose/system emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * - popen() uses temp files, NOT concurrent pipes. The command runs
 *   synchronously before/after the FILE* is used:
 *     "r" mode: command runs FIRST, output saved to temp file, then read
 *     "w" mode: writes go to temp file, command runs on pclose()
 * - system() uses dos.library/SystemTagList() -- synchronous execution
 * - No SIGCHLD or wait() integration
 * - No concurrent execution -- command blocks the calling process
 * - Temp files created in T: (RAM:T/) -- limited by available RAM
 * - Maximum 16 simultaneous popen() streams
 *
 * See docs/posix-tiers.md "Tier 2 -- popen()" for full details.
 */

#ifndef AMIPORT_EMU_POPEN_H
#define AMIPORT_EMU_POPEN_H

#include <stdio.h>

#ifndef AMIPORT_EMU_POPEN_ENABLED
#define AMIPORT_EMU_POPEN_ENABLED 1
#endif

#if AMIPORT_EMU_POPEN_ENABLED

/*
 * amiport_emu_popen() -- Run a command with piped I/O via temp files.
 *
 * For "r" mode: runs the command immediately, captures stdout to a temp
 * file, returns a FILE* for reading the output.
 *
 * For "w" mode: returns a FILE* for writing. On pclose(), the written
 * data is fed as stdin to the command.
 *
 * Returns: FILE* on success, NULL on error (sets errno)
 */
FILE *amiport_emu_popen(const char *command, const char *mode);

/*
 * amiport_emu_pclose() -- Close a popen'd stream and get exit status.
 *
 * For "w" mode streams, this triggers the deferred command execution.
 *
 * Returns: command exit status on success, -1 on error
 */
int amiport_emu_pclose(FILE *stream);

/*
 * amiport_emu_system() -- Execute a shell command synchronously.
 *
 * Uses dos.library/SystemTagList() to run the command.
 *
 * Returns: command exit status, or -1 if command could not be executed
 */
int amiport_emu_system(const char *command);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_POPEN_MACROS
#define popen(cmd, mode) amiport_emu_popen((cmd), (mode))
#define pclose(stream)   amiport_emu_pclose((stream))
/* system() is already available in libnix but may not work correctly
 * with -noixemul. Override only if AMIPORT_EMU_SYSTEM_OVERRIDE is set. */
#ifdef AMIPORT_EMU_SYSTEM_OVERRIDE
#define system(cmd)      amiport_emu_system((cmd))
#endif
#endif

#endif /* AMIPORT_EMU_POPEN_ENABLED */

#endif /* AMIPORT_EMU_POPEN_H */
