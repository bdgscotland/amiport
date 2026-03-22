/*
 * amiport/stdlib.h — stdlib shim for AmigaOS
 *
 * Overrides exit() to avoid the libnix atexit hang (crash-patterns #9).
 * libnix's exit() calls atexit handlers that deadlock when closing
 * console-connected stdio streams on real AmigaOS hardware and FS-UAE.
 * amiport_exit() flushes stdio and calls _exit() directly.
 */

#ifndef AMIPORT_STDLIB_H
#define AMIPORT_STDLIB_H

#include <stdlib.h>

/*
 * amiport_exit — Safe process termination for AmigaOS
 *
 * Flushes stdout and stderr, then calls _exit() to bypass libnix's
 * atexit handlers which hang on console I/O cleanup.
 *
 * This replaces ALL exit() calls in ported programs. On AmigaOS with
 * -noixemul, there is no automatic process cleanup, so ported programs
 * must explicitly free resources before calling exit anyway. Skipping
 * atexit handlers is safe because:
 *   1. The shim doesn't register any atexit handlers
 *   2. Ported programs do explicit cleanup before exit
 *   3. The only atexit handlers are libnix internals (which hang)
 */
void amiport_exit(int status);

/* Drop-in replacement macro */
#ifndef AMIPORT_NO_STDLIB_MACROS
#define exit(s) amiport_exit(s)
#endif

#endif /* AMIPORT_STDLIB_H */
