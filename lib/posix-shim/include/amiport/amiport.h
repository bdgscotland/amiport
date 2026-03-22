/*
 * amiport/amiport.h — Main amiport POSIX shim header
 *
 * Include this for all amiport shim functionality, or include
 * individual headers (amiport/unistd.h, etc.) for specific modules.
 */

#ifndef AMIPORT_H
#define AMIPORT_H

#include <amiport/stdlib.h>
#include <amiport/unistd.h>
#include <amiport/dirent.h>
#include <amiport/getopt.h>
#include <amiport/signal.h>
#include <amiport/errno_map.h>
#include <amiport/err.h>
#include <amiport/sys/stat.h>
#include <amiport/sys/time.h>
#include <amiport/fnmatch.h>
#include <amiport/scandir.h>
#include <amiport/string.h>
#include <amiport/stdio_ext.h>

/* Include Tier 2 emulation headers when requested.
 * Define AMIPORT_INCLUDE_EMU before including this header to get
 * select, mmap, pipe, alarm, and regex emulation. */
#ifdef AMIPORT_INCLUDE_EMU
#include <amiport-emu/amiport-emu.h>
#endif

#endif /* AMIPORT_H */
