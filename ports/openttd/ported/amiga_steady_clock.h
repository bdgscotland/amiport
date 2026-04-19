/*
 * amiga_steady_clock.h -- replace std::chrono::steady_clock::now() with
 * SDL_GetTicks() on AmigaOS to dodge the bebbo-gcc/libstdc++ chrono
 * resolution pitfall.
 *
 * Background (PDR-015, 2026-04-17): bebbo-gcc 13.3 libstdc++ steady_clock
 * falls back to libnix time(2)/gettimeofday() which has 1-second resolution
 * (or 20ms VBLANK at best). OpenTTD's video_driver schedules game and draw
 * ticks via std::chrono::steady_clock::now() at sub-millisecond resolution.
 * With 1-second clock granularity, every tick is delayed by up to 1 second
 * -- explaining the ~2 fps simulation rate even when render frame time is
 * ~22 ms.
 *
 * Fix: swap steady_clock::now() for SDL_GetTicks() which uses
 * timer.device/ReadEClock with microsecond resolution. The C&C port
 * (Vanilla Conquer) verified this lifts simulation from 1 fps to 15 fps.
 *
 * Both std::chrono::steady_clock::now() and SDL_GetTicks() are monotonic;
 * their absolute epochs differ but only deltas matter in OpenTTD's tick
 * scheduling, so the time_point we construct here is comparable with
 * itself across calls. Mixing with real std::chrono::steady_clock::now()
 * elsewhere in the program would break (different epochs) -- so any TU
 * that uses this MUST consistently use amiga_steady_now() and never
 * std::chrono::steady_clock::now() side by side.
 *
 * KB pitfall: known-pitfalls.md "std::chrono::system_clock has poor
 * resolution on bebbo-gcc 6.5" (likely also affects 13.3 -- this fix
 * confirms or refutes).
 */
#ifndef AMIGA_STEADY_CLOCK_H
#define AMIGA_STEADY_CLOCK_H

#include <chrono>
#include <SDL.h>

namespace amiport {
inline std::chrono::steady_clock::time_point amiga_steady_now() noexcept
{
	return std::chrono::steady_clock::time_point(
		std::chrono::milliseconds(SDL_GetTicks()));
}
}

#define AMIGA_STEADY_NOW() ::amiport::amiga_steady_now()

#endif /* AMIGA_STEADY_CLOCK_H */
