/*
 * amiga_paths.h -- AmigaOS path definitions for patch
 * amiport: created for AmigaOS 3.x port
 */

#ifndef AMIGA_PATHS_H
#define AMIGA_PATHS_H

/* amiport: /tmp -> T: assign (maps to RAM:T/).
 * Some environments may not have T: assigned; fall back to RAM: */
#define _PATH_TMP       "T:"
#define _PATH_TMP_FALLBACK "RAM:"

/* amiport: /dev/tty -> * (AmigaOS console, current window) */
#define _PATH_TTY       "*"

/* amiport: /dev/null -> NIL: (AmigaOS bit bucket) */
#define _PATH_DEVNULL   "NIL:"

#endif /* AMIGA_PATHS_H */
