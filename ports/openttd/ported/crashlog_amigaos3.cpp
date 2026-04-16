/*
 * crashlog_amigaos3.cpp -- AmigaOS 3.x crash logger for OpenTTD
 *
 * Replaces src/os/unix/crashlog_unix.cpp. Minimal implementation
 * since AmigaOS has no POSIX signals, backtrace, or uname.
 */

#include "../original/OpenTTD-13.4/src/stdafx.h"
#include "../original/OpenTTD-13.4/src/crashlog.h"
#include "../original/OpenTTD-13.4/src/string_func.h"
#include "../original/OpenTTD-13.4/src/gamelog.h"
#include "../original/OpenTTD-13.4/src/saveload/saveload.h"

#include "../original/OpenTTD-13.4/src/safeguards.h"

class CrashLogAmiga : public CrashLog {
	char *LogOSVersion(char *buffer, const char *last) const override
	{
		return buffer + seprintf(buffer, last,
				"Operating system:\n"
				" Name:     AmigaOS\n"
				" Release:  3.x\n"
				" Version:  Kickstart 3.1+\n"
				" Machine:  m68k (68040+)\n"
		);
	}

	char *LogError(char *buffer, const char *last, const char *message) const override
	{
		return buffer + seprintf(buffer, last,
				"Crash reason:\n"
				" Message: %s\n\n",
				message == nullptr ? "<none>" : message
		);
	}

	char *LogStacktrace(char *buffer, const char *last) const override
	{
		return buffer + seprintf(buffer, last,
				"Stacktrace:\n"
				" Not available on AmigaOS.\n\n"
		);
	}

public:
	CrashLogAmiga() {}
};

/* static */ void CrashLog::InitialiseCrashLog()
{
	/* No signal handlers on AmigaOS -- Guru Meditations are handled
	 * by the OS, not by user-space signal handlers. */
}

/* static */ void CrashLog::InitThread()
{
}
