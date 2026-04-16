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

#include "../original/OpenTTD-13.4/src/safeguards.h"

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
	for (int i = 0; i < argc; i++) StrMakeValidInPlace(argv[i]);

	CrashLog::InitialiseCrashLog();

	SetRandomSeed(time(nullptr));

	return openttd_main(argc, argv);
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
