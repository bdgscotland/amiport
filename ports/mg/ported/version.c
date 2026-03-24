/*	$OpenBSD: version.c,v 1.9 2005/06/14 18:14:40 kjell Exp $	*/

/* This file is in the public domain. */

/*
 * This file contains the string that gets written
 * out by the emacs-version command.
 */

#ifndef __AMIGA__
#include <signal.h> /* amiport: sig_atomic_t defined as int in tty.c on AmigaOS */
#endif
#include <stdio.h>

#include "def.h"

const char	version[] = PACKAGE_STRING;

/*
 * Display the version. All this does
 * is copy the version string onto the echo line.
 */
int
showversion(int f, int n)
{
	ewprintf("%s", version);
	return (TRUE);
}
