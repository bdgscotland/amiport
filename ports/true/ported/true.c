/*	$OpenBSD: true.c,v 1.1 2015/11/11 19:05:28 deraadt Exp $	*/

/* Public domain - Theo de Raadt */

/* amiport: include amiport/stdlib.h to activate exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: true 1.1 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

int
main(int argc, char *argv[])
{
	return (0);
}
