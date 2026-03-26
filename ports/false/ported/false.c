/*	$OpenBSD: false.c,v 1.1 2015/11/11 19:05:28 deraadt Exp $	*/

/* Public domain - Theo de Raadt */

/* amiport: stdlib.h provides exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>

/* amiport: Amiga version string */
static const char *verstag __attribute__((used)) = "$VER: false 1.1 (26.03.2026)";

/* amiport: stack cookie */
long __stack = 8192;

int
main(int argc, char *argv[])
{
	/* amiport: exit(1) is invisible to AmigaDOS scripts; RETURN_ERROR (10)
	 * is what IF ERROR tests for -- this is the whole point of the program */
	return (10);
}
