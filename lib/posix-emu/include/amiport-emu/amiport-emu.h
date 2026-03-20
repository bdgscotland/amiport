/*
 * amiport-emu/amiport-emu.h — Main amiport Tier 2 emulation header
 *
 * Include this for all Tier 2 emulation functionality, or include
 * individual headers (amiport-emu/select.h, etc.) for specific modules.
 *
 * EMULATION NOTICE: Functions in this library approximate POSIX semantics
 * but have documented behavioural differences. Each header lists its
 * specific caveats. See docs/posix-tiers.md for the full classification.
 */

#ifndef AMIPORT_EMU_H
#define AMIPORT_EMU_H

#include <amiport-emu/select.h>
#include <amiport-emu/mmap.h>
#include <amiport-emu/pipe.h>
#include <amiport-emu/alarm.h>

#endif /* AMIPORT_EMU_H */
