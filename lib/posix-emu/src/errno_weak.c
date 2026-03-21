/* errno_weak.c — Weak errno for libamiport-emu.a
 * Same fix as lib/posix-shim/src/errno_map.c — see that file for explanation.
 */
#include <errno.h>

#ifdef __libnix__
#ifdef errno
static int __amiport_emu_errno_fallback = 0;
extern int * __errno;
int * __attribute__((weak)) __errno = &__amiport_emu_errno_fallback;
#else
int __attribute__((weak)) errno;
#endif
#endif
