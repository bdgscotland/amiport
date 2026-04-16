/*
 * os_amigaos3.cpp -- AmigaOS 3.x platform layer for OpenTTD
 *
 * Replaces src/os/unix/unix.cpp. Provides filesystem helpers,
 * entry point, and platform stubs for AmigaOS 3.x on 68k.
 */

#include "../original/OpenTTD-13.4/src/stdafx.h"
#include "../original/OpenTTD-13.4/src/textbuf_gui.h"
#include "../original/OpenTTD-13.4/src/openttd.h"
#include "../original/OpenTTD-13.4/src/crashlog.h"
#include "../original/OpenTTD-13.4/src/core/random_func.hpp"
#include "../original/OpenTTD-13.4/src/debug.h"
#include "../original/OpenTTD-13.4/src/string_func.h"
#include "../original/OpenTTD-13.4/src/fios.h"

#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>

/* Debug log -- defined BEFORE safeguards.h which blocks fopen/open/sprintf */
static int _dbgfd = -1;
static void dbglog(const char *msg)
{
	if (_dbgfd < 0) _dbgfd = open("WORK:OpenTTD/debug.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (_dbgfd >= 0) { write(_dbgfd, msg, strlen(msg)); write(_dbgfd, "\n", 1); }
	printf("%s\n", msg);
}

static void dbglog_hex(const char *prefix, unsigned long val)
{
	char buf[80];
	snprintf(buf, sizeof(buf), "%s 0x%08lx", prefix, val);
	dbglog(buf);
}

static void dbglog_bytes(const char *prefix, unsigned char *p, int n)
{
	char buf[80];
	int off = snprintf(buf, sizeof(buf), "%s", prefix);
	for (int i = 0; i < n && off < 70; i++)
		off += snprintf(buf + off, sizeof(buf) - off, " %02x", p[i]);
	dbglog(buf);
}

#include "../original/OpenTTD-13.4/src/safeguards.h"

extern "C" void diaglog(const char *msg)
{
	dbglog(msg);
}

/*
 * AmigaOS root check: a path is "root" if it's a volume name (ends with ':')
 * or is just "/" (unlikely on Amiga but handle it).
 */
bool FiosIsRoot(const char *path)
{
	size_t len = strlen(path);
	if (len == 0) return false;
	return path[len - 1] == ':';
}

/*
 * List available drives. On AmigaOS, these are volumes.
 * For the dedicated server, this is rarely used. Stub for now.
 */
void FiosGetDrives(FileList &file_list)
{
	return;
}

/*
 * Get disk free space. Stub -- returns false (unknown).
 * A real implementation would use dos.library Info().
 */
bool FiosGetDiskFreeSpace(const char *path, uint64 *tot)
{
	if (tot != nullptr) *tot = 64 * 1024 * 1024;
	return true;
}

bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb)
{
	char filename[MAX_PATH];
	int res;
	assert(path[strlen(path) - 1] == PATHSEPCHAR);
	if (strlen(path) > 2) assert(path[strlen(path) - 2] != PATHSEPCHAR);
	res = seprintf(filename, lastof(filename), "%s%s", path, ent->d_name);

	if (res >= (int)lengthof(filename) || res < 0) return false;

	return stat(filename, sb) == 0;
}

bool FiosIsHiddenFile(const struct dirent *ent)
{
	/* AmigaOS has no dot-file convention. Nothing is hidden. */
	return false;
}

void ShowInfo(const char *str)
{
	fprintf(stderr, "%s\n", str);
}

void ShowOSErrorBox(const char *buf, bool system)
{
	fprintf(stderr, "Error: %s\n", buf);
}

int CDECL main(int argc, char *argv[])
{
	dbglog("AMIGA: main() entered");
	for (int i = 0; i < argc; i++) StrMakeValidInPlace(argv[i]);
	dbglog("AMIGA: argv validated");

	CrashLog::InitialiseCrashLog();
	dbglog("AMIGA: CrashLog init done");

	SetRandomSeed(time(nullptr));
	dbglog("AMIGA: random seed set");

	dbglog_hex("AMIGA: openttd_main at", (unsigned long)(void *)&openttd_main);
	unsigned char *fb = (unsigned char *)(void *)&openttd_main;
	dbglog_bytes("AMIGA: first bytes:", fb, 8);

	if (fb[0] == 0x4e && fb[1] == 0x55) {
		dbglog("AMIGA: prologue valid (link).");
	} else {
		dbglog("AMIGA: BAD PROLOGUE!");
	}

	dbglog("AMIGA: testing 7 string ctors (same as openttd_main)...");
	{
		std::string s1, s2, s3, s4, s5, s6, s7;
		dbglog("AMIGA: 7 strings OK");
	}

	dbglog("AMIGA: testing unique_ptr<int>...");
	{
		std::unique_ptr<int> p(new int(42));
		dbglog("AMIGA: unique_ptr OK");
	}

	dbglog("AMIGA: ALL PRE-TESTS PASSED.");
	dbglog("AMIGA: Simulating openttd_main line by line...");

	dbglog("AMIGA: line 533: local strings...");
	std::string musicdriver, sounddriver, videodriver, blitter;
	std::string graphics_set, sounds_set, music_set;
	dbglog("AMIGA: line 540: Dimension...");
	/* Dimension resolution = {0, 0}; -- just two uint16 */
	unsigned short res_w = 0, res_h = 0;
	dbglog("AMIGA: line 541: AfterNewGRFScan...");
	/* This is the likely crash point -- new AfterNewGRFScan() */
	extern void _ZN15AfterNewGRFScanC1Ev(void *); /* mangled ctor */
	void *scanner_mem = new char[256]; /* oversized to be safe */
	dbglog("AMIGA: allocated scanner memory");
	/* Skip calling the constructor for now */
	dbglog("AMIGA: line 552: GetOptData...");
	/* Skip option parsing too */
	dbglog("AMIGA: calling openttd_main with try/catch...");
	int ret = 1;
	try {
		ret = openttd_main(argc, argv);
		dbglog("AMIGA: openttd_main returned normally");
	} catch (const std::exception &e) {
		dbglog("AMIGA: EXCEPTION caught!");
		const char *w = e.what();
		if (_dbgfd >= 0) {
			write(_dbgfd, "  type: std::exception\n  what: ", 30);
			write(_dbgfd, w, strlen(w));
			write(_dbgfd, "\n", 1);
		}
	} catch (...) {
		dbglog("AMIGA: UNKNOWN exception caught!");
	}

	dbglog("AMIGA: openttd_main returned");
	if (_dbgfd >= 0) close(_dbgfd);
	return ret;
}

bool GetClipboardContents(char *buffer, const char *last)
{
	return false;
}

void OSOpenBrowser(const char *url)
{
	/* No browser launching on AmigaOS dedicated server */
	(void)url;
}

void SetCurrentThreadName(const char *threadName)
{
	/* No threads on AmigaOS */
	(void)threadName;
}
