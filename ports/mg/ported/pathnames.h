/*	$OpenBSD$	*/

/* This file is in the public domain. */

/*
 *	standard path names
 */

/* amiport: use S: assign for startup files on AmigaOS */
#ifdef __AMIGA__
#define	_PATH_MG_DIR		"S:.mg.d"
#define	_PATH_MG_STARTUP	"S:.mg"
#define	_PATH_MG_TERM		"S:.mg-%s"
#else
#define	_PATH_MG_DIR		"~/.mg.d"
#define	_PATH_MG_STARTUP	"%s/.mg"
#define	_PATH_MG_TERM		"%s/.mg-%s"
#endif
